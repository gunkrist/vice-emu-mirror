/** \file   monitor_binary.c
 *  \brief  Monitor implementation - binary network access
 *
 *  \author EmpathicQubit <empathicqubit@entan.gl>
 */

/*
 * This file is part of VICE, the Versatile Commodore Emulator.
 * See README for copyright notice.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307  USA.
 *
 */

#include "vice.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "cmdline.h"
#include "lib.h"
#include "log.h"
#include "monitor.h"
#include "monitor_binary.h"
#include "montypes.h"
#include "resources.h"
#include "uiapi.h"
#include "util.h"
#include "vicesocket.h"

#include "mon_breakpoint.h"
#include "mon_file.h"
#include "mon_register.h"

#ifdef HAVE_NETWORK

#define ADDR_LIMIT(x) ((uint16_t)(addr_mask(x)))

static vice_network_socket_t * listen_socket = NULL;
static vice_network_socket_t * connected_socket = NULL;

static char *monitor_binary_server_address = NULL;
static int monitor_binary_enabled = 0;

enum t_binary_command {
    e_MON_CMD_INVALID = 0x00,

    e_MON_CMD_MEM_GET = 0x01,
    e_MON_CMD_MEM_SET = 0x02,

    e_MON_CMD_CHECKPOINT_GET = 0x11,
    e_MON_CMD_CHECKPOINT_SET = 0x12,
    e_MON_CMD_CHECKPOINT_DELETE = 0x13,
    e_MON_CMD_CHECKPOINT_LIST = 0x14,
    e_MON_CMD_CHECKPOINT_TOGGLE = 0x15,

    e_MON_CMD_CONDITION_SET = 0x22,

    e_MON_CMD_REGISTERS_GET = 0x31,
    e_MON_CMD_REGISTERS_SET = 0x32,

    e_MON_CMD_ADVANCE_INSTRUCTIONS = 0x71,
    e_MON_CMD_KEYBOARD_FEED = 0x72,
    e_MON_CMD_EXECUTE_UNTIL_RETURN = 0x73,

    e_MON_CMD_PING = 0x81,
    e_MON_CMD_BANKS_AVAILABLE = 0x82,
    e_MON_CMD_REGISTERS_AVAILABLE = 0x83,

    e_MON_CMD_EXIT = 0xaa,
    e_MON_CMD_QUIT = 0xbb,
    e_MON_CMD_RESET = 0xcc,
    e_MON_CMD_AUTOSTART = 0xdd,
};
typedef enum t_binary_command BINARY_COMMAND;

enum t_binary_response {
    e_MON_RESPONSE_MEM_GET = 0x01,
    e_MON_RESPONSE_MEM_SET = 0x02,

    e_MON_RESPONSE_CHECKPOINT_INFO = 0x11,

    e_MON_RESPONSE_CHECKPOINT_DELETE = 0x13,
    e_MON_RESPONSE_CHECKPOINT_LIST = 0x14,
    e_MON_RESPONSE_CHECKPOINT_TOGGLE = 0x15,

    e_MON_RESPONSE_CONDITION_SET = 0x22,

    e_MON_RESPONSE_REGISTER_INFO = 0x31,

    e_MON_RESPONSE_JAM = 0x61,
    e_MON_RESPONSE_STOPPED = 0x62,
    e_MON_RESPONSE_RESUMED = 0x63,

    e_MON_RESPONSE_ADVANCE_INSTRUCTIONS = 0x71,
    e_MON_RESPONSE_KEYBOARD_FEED = 0x72,
    e_MON_RESPONSE_EXECUTE_UNTIL_RETURN = 0x73,

    e_MON_RESPONSE_PING = 0x81,
    e_MON_RESPONSE_BANKS_AVAILABLE = 0x82,
    e_MON_RESPONSE_REGISTERS_AVAILABLE = 0x83,

    e_MON_RESPONSE_EXIT = 0xaa,
    e_MON_RESPONSE_QUIT = 0xbb,
    e_MON_RESPONSE_RESET = 0xcc,
    e_MON_RESPONSE_AUTOSTART = 0xdd,
};
typedef enum t_binary_response BINARY_RESPONSE;

enum t_mon_error {
    e_MON_ERR_OK = 0x00,
    e_MON_ERR_CMD_INVALID_LENGTH = 0x80,
    e_MON_ERR_INVALID_PARAMETER = 0x81,
};
typedef enum t_mon_error BINARY_ERROR;

struct binary_command_s {
    unsigned char *body;
    uint32_t length;
    uint32_t request_id;
    uint8_t api_version;
    BINARY_COMMAND type;
};
typedef struct binary_command_s binary_command_t;

int monitor_binary_transmit(const unsigned char *buffer, size_t buffer_length)
{
    int error = 0;

    if (connected_socket) {
        size_t len = (size_t)vice_network_send(connected_socket, buffer, buffer_length, 0);

        if (len != buffer_length) {
            error = -1;
        } else {
            error = (int)len;
        }
    }

    return error;
}

static void monitor_binary_quit(void)
{
    vice_network_socket_close(connected_socket);
    connected_socket = NULL;
}

int monitor_binary_receive(unsigned char *buffer, size_t buffer_length)
{
    int count = 0;

    do {
        if (!connected_socket) {
            break;
        }

        count = vice_network_receive(connected_socket, buffer, buffer_length, 0);

        if (count < 0) {
            log_message(LOG_DEFAULT, "monitor_binary_receive(): vice_network_receive() returned -1, breaking connection");
            monitor_binary_quit();
        }
    } while (0);

    return count;
}

static int monitor_binary_data_available(void)
{
    int available = 0;

    if (connected_socket != NULL) {
        available = vice_network_select_poll_one(connected_socket);
    } else if (listen_socket != NULL) {
        /* we have no connection yet, allow for connection */

        if (vice_network_select_poll_one(listen_socket)) {
            connected_socket = vice_network_accept(listen_socket);
        }
    }


    return available;
}

void monitor_check_binary(void)
{
    if (monitor_binary_data_available()) {
        monitor_startup_trap();
    }
}

#define ASC_STX 0x02

#define MON_BINARY_API_VERSION 0x01

#define MON_EVENT_ID 0xffffffff

/*! \internal \brief Write uint16 to buffer and return pointer to byte after */
static unsigned char *write_uint16(uint16_t input, unsigned char *output) {
    output[0] = input & 0xFFu;
    output[1] = (input >> 8) & 0xFFu;

    return output + 2;
}

/*! \internal \brief Write uint32 to buffer and return pointer to byte after */
static unsigned char *write_uint32(uint32_t input, unsigned char *output) {
    output[0] = input & 0xFFu;
    output[1] = (input >> 8) & 0xFFu;
    output[2] = (input >> 16) & 0xFFu;
    output[3] = (uint8_t)(input >> 24) & 0xFFu;

    return output + 4;
}

/*! \internal \brief Write string to buffer and return pointer to byte after */
static unsigned char *write_string(uint8_t length, unsigned char *input, unsigned char *output) {
    output[0] = length;
    memcpy(&output[1], input, length);

    return output + length + 1;
}

/*! \internal \brief Read 32bit little endian value from buffer to uint32 value */
static uint32_t little_endian_to_uint32(unsigned char *input) {
    return (input[3] << 24) + (input[2] << 16) + (input[1] << 8) + input[0];
}

/*! \internal \brief Read 16bit little endian value from buffer to uint16 value */
static uint16_t little_endian_to_uint16(unsigned char *input) {
    return (input[1] << 8) + input[0];
}

static void monitor_binary_response(uint32_t length, BINARY_RESPONSE response_type, BINARY_ERROR errorcode, uint32_t request_id, unsigned char *body)
{
    unsigned char response[12];

    response[0] = ASC_STX;
    response[1] = MON_BINARY_API_VERSION;
    write_uint32(length, &response[2]);
    response[6] = (uint8_t)response_type;
    response[7] = (uint8_t)errorcode;
    write_uint32(request_id, &response[8]);

    monitor_binary_transmit(response, sizeof response);

    if (body != NULL) {
        monitor_binary_transmit(body, length);
    }
}

static void monitor_binary_response_stopped(uint32_t request_id)
{
    unsigned char response[2];
    uint16_t addr = ((uint16_t)((monitor_cpu_for_memspace[e_comp_space]->mon_register_get_val)(e_comp_space, e_PC)));

    write_uint16(addr, response);

    monitor_binary_response(2, e_MON_RESPONSE_STOPPED, e_MON_ERR_OK, MON_EVENT_ID, response);
}

static void monitor_binary_response_resumed(uint32_t request_id)
{
    unsigned char response[2];
    uint16_t addr = ((uint16_t)((monitor_cpu_for_memspace[e_comp_space]->mon_register_get_val)(e_comp_space, e_PC)));

    write_uint16(addr, response);

    monitor_binary_response(2, e_MON_RESPONSE_RESUMED, e_MON_ERR_OK, MON_EVENT_ID, response);
}

ui_jam_action_t monitor_binary_ui_jam_dialog(const char *format, ...)
{
    unsigned char response[2];
    uint16_t addr = ((uint16_t)((monitor_cpu_for_memspace[e_comp_space]->mon_register_get_val)(e_comp_space, e_PC)));

    write_uint16(addr, response);

    monitor_binary_response(0, e_MON_RESPONSE_JAM, e_MON_ERR_OK, MON_EVENT_ID, response);

    return UI_JAM_MONITOR;
}

static bool ignore_fake_register(mon_reg_list_t *reg)
{
    return reg->flags & (MON_REGISTER_IS_FLAGS | MON_REGISTER_IS_MEMORY);
}

void monitor_binary_response_register_info(uint32_t request_id)
{
    unsigned char *response;
    uint16_t count = 0;
    uint32_t response_size = 2;
    uint8_t item_size = 3;
    /* FIXME: Should I add the memspace to the request? */
    mon_reg_list_t *regs = mon_register_list_get(e_comp_space);
    mon_reg_list_t *regs_cursor = regs;
    unsigned char *response_cursor;

    for( ; regs_cursor->name ; regs_cursor++) {
        if (!ignore_fake_register(regs_cursor)) {
            ++count;
        }
    }

    response_size += count * (item_size + 1);
    response = lib_malloc(response_size);
    response_cursor = response;

    regs_cursor = regs;

    response_cursor = write_uint16(count, response_cursor);

    for( ; regs_cursor->name ; regs_cursor++) {
        if (ignore_fake_register(regs_cursor)) {
            continue;
        }

        *response_cursor = item_size;
        ++response_cursor;

        *response_cursor = regs_cursor->id;
        ++response_cursor;

        response_cursor = write_uint16((uint16_t)regs_cursor->val, response_cursor);
    }

    monitor_binary_response(response_size, e_MON_RESPONSE_REGISTER_INFO, e_MON_ERR_OK, request_id, response);

    lib_free(response);
}

/*! \internal \brief called when the monitor is opened */
void monitor_binary_event_opened(void) {
    monitor_binary_response_register_info(MON_EVENT_ID);
    monitor_binary_response_stopped(MON_EVENT_ID);
}

/*! \internal \brief called when the monitor is closed */
void monitor_binary_event_closed(void) {
    monitor_binary_response_resumed(MON_EVENT_ID);
}

/*! \internal \brief Responds with information about a checkpoint.

 \param request_id ID of the request

 \param mon_checkpoint_t The checkpoint

 \param hit Is the checkpoint hit in the emulator?
*/
void monitor_binary_response_checkpoint_info(uint32_t request_id, mon_checkpoint_t *checkpt, bool hit) {
    unsigned char response[22];
    MEMORY_OP op = (MEMORY_OP)(
        (checkpt->check_store ? e_store : 0) 
        | (checkpt->check_load ? e_load : 0) 
        | (checkpt->check_exec ? e_exec : 0)
    );

    write_uint32(checkpt->checknum, response);
    response[4] = hit;

    write_uint16((uint16_t)addr_location(checkpt->start_addr), &response[5]);
    write_uint16((uint16_t)addr_location(checkpt->end_addr), &response[7]);
    response[9] = checkpt->stop;
    response[10] = checkpt->enabled;
    response[11] = op;
    response[12] = checkpt->temporary;

    write_uint32((uint32_t)checkpt->hit_count, &response[13]);
    write_uint32((uint32_t)checkpt->ignore_count, &response[17]);
    response[21] = !!checkpt->condition;

    monitor_binary_response(sizeof (response), e_MON_RESPONSE_CHECKPOINT_INFO, e_MON_ERR_OK, request_id, response);
}

static void monitor_binary_error(BINARY_ERROR errorcode, uint32_t request_id)
{
    monitor_binary_response(0, 0, errorcode, request_id, NULL);
}

static void monitor_binary_process_ping(binary_command_t *command)
{
    monitor_binary_response(0, e_MON_RESPONSE_PING, e_MON_ERR_OK, command->request_id, NULL);
}

static void monitor_binary_process_checkpoint_get(binary_command_t *command)
{
    uint32_t brknum = little_endian_to_uint32(command->body);
    mon_checkpoint_t *checkpt;

    if (command->length < sizeof(brknum)) {
        monitor_binary_error(e_MON_ERR_CMD_INVALID_LENGTH, command->request_id);
        return;
    }

    checkpt = mon_breakpoint_find_checkpoint((int)brknum);

    if (!checkpt) {
        monitor_binary_error(e_MON_ERR_INVALID_PARAMETER, command->request_id);
        return;
    }

    monitor_binary_response_checkpoint_info(command->request_id, checkpt, 0);
}

static void monitor_binary_process_checkpoint_set(binary_command_t *command)
{
    int brknum;
    mon_checkpoint_t *checkpt;
    unsigned char *body = command->body;

    if (command->length < 8) {
        monitor_binary_error(e_MON_ERR_CMD_INVALID_LENGTH, command->request_id);
        return;
    }

    brknum = mon_breakpoint_add_checkpoint(
        (MON_ADDR)little_endian_to_uint16(&body[0]),
        (MON_ADDR)little_endian_to_uint16(&body[2]),
        (bool)body[4],
        (MEMORY_OP)body[6],
        (bool)body[7]
        );

    if (!body[5]) {
        mon_breakpoint_switch_checkpoint(e_OFF, brknum);
    }

    checkpt = mon_breakpoint_find_checkpoint(brknum);

    monitor_binary_response_checkpoint_info(command->request_id, checkpt, 0);
}

static void monitor_binary_process_checkpoint_delete(binary_command_t *command)
{
    uint32_t brknum = little_endian_to_uint32(command->body);
    mon_checkpoint_t *checkpt;

    if (command->length < sizeof(brknum)) {
        monitor_binary_error(e_MON_ERR_CMD_INVALID_LENGTH, command->request_id);
        return;
    }

    checkpt = mon_breakpoint_find_checkpoint((int)brknum);

    if (!checkpt) {
        monitor_binary_error(e_MON_ERR_INVALID_PARAMETER, command->request_id);
        return;
    }

    mon_breakpoint_delete_checkpoint((int)brknum);

    monitor_binary_response(0, e_MON_RESPONSE_CHECKPOINT_DELETE, e_MON_ERR_OK, command->request_id, NULL);
}

static void monitor_binary_process_checkpoint_list(binary_command_t *command)
{
    unsigned char response[sizeof(uint32_t)];
    unsigned int i, len;
    uint32_t request_id = command->request_id;
    mon_checkpoint_t **checkpts = mon_breakpoint_checkpoint_list_get(&len);

    for(i = 0; i < len; i++) {
        monitor_binary_response_checkpoint_info(request_id, checkpts[i], 0);
    }

    write_uint32((uint32_t)len, response);

    monitor_binary_response(sizeof(uint32_t), e_MON_RESPONSE_CHECKPOINT_LIST, e_MON_ERR_OK, request_id, response);

    lib_free(checkpts);
}

static void monitor_binary_process_checkpoint_toggle(binary_command_t *command)
{
    uint32_t brknum = little_endian_to_uint32(command->body);
    uint8_t enable = !!command->body[4];
    mon_checkpoint_t *checkpt;
    
    if (command->length < 5) {
        monitor_binary_error(e_MON_ERR_CMD_INVALID_LENGTH, command->request_id);
        return;
    }

    checkpt = mon_breakpoint_find_checkpoint((int)brknum);

    if (!checkpt) {
        monitor_binary_error(e_MON_ERR_INVALID_PARAMETER, command->request_id);
        return;
    }

    mon_breakpoint_switch_checkpoint((int)enable, (int)brknum);

    monitor_binary_response(0, e_MON_RESPONSE_CHECKPOINT_TOGGLE, e_MON_ERR_OK, command->request_id, NULL);
}

static void monitor_binary_process_condition_set(binary_command_t *command)
{
    const char* cmd_fmt = "cond %u if ( %s )";

    mon_checkpoint_t *checkpt;
    unsigned char *cond;
    size_t cmd_length;
    char *cmd;

    unsigned char *body = command->body;
    uint32_t brknum = little_endian_to_uint32(body);
    uint8_t length = body[4];

    if (command->length < 5 + length) {
        monitor_binary_error(e_MON_ERR_CMD_INVALID_LENGTH, command->request_id);
        return;
    }

    checkpt = mon_breakpoint_find_checkpoint(brknum);

    if (!checkpt) {
        monitor_binary_error(e_MON_ERR_INVALID_PARAMETER, command->request_id);
        return;
    }

    cond = &body[5];

    /* This should be changed to memcpy if any other values are added later */
    cond[length] = '\0';

    cmd_length = snprintf(NULL, 0, cmd_fmt, brknum, cond);

    cmd = lib_malloc(cmd_length);

    sprintf(cmd, cmd_fmt, brknum, cond);

    if (parse_and_execute_line(cmd) != 0) {
        monitor_binary_error(e_MON_ERR_INVALID_PARAMETER, command->request_id);
        return;
    }

    monitor_binary_response(0, e_MON_RESPONSE_CONDITION_SET, e_MON_ERR_OK, command->request_id, NULL);

    lib_free(cmd);
}

static void monitor_binary_process_advance_instructions(binary_command_t *command)
{
    uint8_t step_over_subroutines = command->body[0];
    uint16_t count = little_endian_to_uint16(&command->body[1]);

    if (command->length < 3) {
        monitor_binary_error(e_MON_ERR_CMD_INVALID_LENGTH, command->request_id);
        return;
    }

    if (step_over_subroutines) {
        mon_instructions_next(count);
    } else {
        mon_instructions_step(count);
    }

    monitor_binary_response(0, e_MON_RESPONSE_ADVANCE_INSTRUCTIONS, e_MON_ERR_OK, command->request_id, NULL);
}

static void monitor_binary_process_reset(binary_command_t *command)
{
    uint8_t reset_type = command->body[0];

    if (command->length < 1) {
        monitor_binary_error(e_MON_ERR_CMD_INVALID_LENGTH, command->request_id);
        return;
    }

    mon_reset_machine((int)reset_type);

    monitor_binary_response(0, e_MON_RESPONSE_RESET, e_MON_ERR_OK, command->request_id, NULL);
}

static void monitor_binary_process_keyboard_feed(binary_command_t *command)
{
    unsigned char *body = command->body;
    uint8_t length = body[0];

    if(command->length < 1 + length) {
        monitor_binary_error(e_MON_ERR_CMD_INVALID_LENGTH, command->request_id);
        return;
    }

    body[1 + length] = '\0';

    mon_keyboard_feed((char *)&body[1]);

    monitor_binary_response(0, e_MON_RESPONSE_KEYBOARD_FEED, e_MON_ERR_OK, command->request_id, NULL);
}

static void monitor_binary_process_execute_until_return(binary_command_t *command)
{
    mon_instruction_return();

    monitor_binary_response(0, e_MON_RESPONSE_EXECUTE_UNTIL_RETURN, e_MON_ERR_OK, command->request_id, NULL);
}

static void monitor_binary_process_autostart(binary_command_t *command)
{
    unsigned char *body = command->body;
    uint8_t run = !!body[0];
    uint16_t file_index = little_endian_to_uint16(&body[1]);
    uint8_t filename_length = body[3];
    unsigned char* filename = &body[4];

    if(command->length < 4 + filename_length) {
        monitor_binary_error(e_MON_ERR_CMD_INVALID_LENGTH, command->request_id);
        return;
    }

    /* This should be changed later if other fields are added after it */
    filename[filename_length] = '\0';

    if(mon_autostart((char *)filename, file_index, run) < 0) {
        monitor_binary_error(e_MON_ERR_INVALID_PARAMETER, command->request_id);
        return;
    }

    monitor_binary_response(0, e_MON_RESPONSE_AUTOSTART, e_MON_ERR_OK, command->request_id, NULL);
}

static void monitor_binary_process_registers_get(binary_command_t *command)
{
    monitor_binary_response_register_info(command->request_id);
}

static void monitor_binary_process_registers_set(binary_command_t *command)
{
    const int header_size = 2;
    unsigned int i = 0;
    unsigned char *body = command->body;
    unsigned char *body_cursor = body;
    uint16_t count = little_endian_to_uint16(body);

    if (command->length < header_size + count * (3 + 1)) {
        monitor_binary_error(e_MON_ERR_CMD_INVALID_LENGTH, command->request_id);
        return;
    }

    body_cursor += header_size;

    for (i = 0; i < count; i++) {
        uint8_t item_size = body_cursor[0];
        uint8_t reg_id = body_cursor[1];
        uint16_t reg_val = little_endian_to_uint16(&body_cursor[2]);

        if (item_size < 3) {
            monitor_binary_error(e_MON_ERR_INVALID_PARAMETER, command->request_id);
            return;
        }

        if (!mon_register_valid(e_comp_space, (int)reg_id)) {
            monitor_binary_error(e_MON_ERR_INVALID_PARAMETER, command->request_id);
            return;
        }

        monitor_cpu_for_memspace[e_comp_space]->mon_register_set_val(e_comp_space, reg_regid(reg_id), reg_val);

        body_cursor += item_size + 1;
    }

    monitor_binary_response_register_info(command->request_id);
}

static void monitor_binary_process_exit(binary_command_t *command)
{
    exit_mon = 1;

    monitor_binary_response(0, e_MON_RESPONSE_EXIT, e_MON_ERR_OK, command->request_id, NULL);
}

static void monitor_binary_process_quit(binary_command_t *command)
{
    mon_quit();

    monitor_binary_response(0, e_MON_RESPONSE_QUIT, e_MON_ERR_OK, command->request_id, NULL);
}

static void monitor_binary_process_banks_available(binary_command_t *command)
{
    unsigned char *response;
    unsigned char *response_cursor;
    const int *banknums;
    const char **banknames;
    size_t *item_sizes;
    uint16_t count;

    unsigned int i = 0;
    uint32_t response_size = 2;

    if (
        !mon_interfaces[e_comp_space]->mem_bank_list
        || !mon_interfaces[e_comp_space]->mem_bank_list_nos
        ) {
            /* TODO: Better error codes? */
            monitor_binary_error(e_MON_ERR_INVALID_PARAMETER, command->request_id);
            return;
        }

    banknums = mon_interfaces[e_comp_space]->mem_bank_list_nos();
    banknames = mon_interfaces[e_comp_space]->mem_bank_list();

    while (banknames[i]) {
        ++i;
    }

    count = i;

    item_sizes = lib_malloc(sizeof(size_t) * count);

    for (i = 0; i < count; i++) {
        item_sizes[i] = strlen(banknames[i]) + 3;
        response_size += item_sizes[i] + 1;
    }

    response = lib_malloc(response_size);
    response_cursor = response;

    response_cursor = write_uint16(count, response_cursor);

    for (i = 0; i < count; i++) {
        size_t item_size = item_sizes[i];
        *response_cursor = item_size;
        ++response_cursor;

        response_cursor = write_uint16(banknums[i], response_cursor);

        response_cursor = write_string(item_size - 3, (unsigned char *)banknames[i], response_cursor);
    }

    monitor_binary_response(response_size, e_MON_RESPONSE_BANKS_AVAILABLE, e_MON_ERR_OK, command->request_id, response);

    lib_free(response);
    lib_free(item_sizes);
}

static void monitor_binary_process_registers_available(binary_command_t *command)
{
    unsigned char *response;
    unsigned char *response_cursor;
    size_t *item_sizes;
    uint16_t count_all;
    uint16_t count_response = 0;
    unsigned int i = 0;
    uint32_t response_size = 2;
    mon_reg_list_t *regs = mon_register_list_get(e_comp_space);

    while (regs[i].name) {
        ++i;

        if(!ignore_fake_register(&regs[i])) {
            ++count_response;
        }
    }

    count_all = i;

    item_sizes = lib_malloc(sizeof(size_t) * count_all);

    for (i = 0; i < count_all; i++) {
        if(ignore_fake_register(&regs[i])) {
            continue;
        }

        item_sizes[i] = strlen(regs[i].name) + 3;
        response_size += item_sizes[i] + 1;
    }

    response = lib_malloc(response_size);
    response_cursor = response;

    i = 0;

    response_cursor = write_uint16(count_response, response_cursor);

    for (i = 0; i < count_all; i++) {
        mon_reg_list_t *reg = &regs[i];
        size_t item_size = item_sizes[i];

        if(ignore_fake_register(reg)) {
            continue;
        }

        *response_cursor = item_size;
        ++response_cursor;

        *response_cursor = reg->id;
        ++response_cursor;

        *response_cursor = reg->size;
        ++response_cursor;

        response_cursor = write_string(item_size - 3, (unsigned char *)reg->name, response_cursor);
    }

    monitor_binary_response(response_size, e_MON_RESPONSE_REGISTERS_AVAILABLE, e_MON_ERR_OK, command->request_id, response);

    lib_free(response);
    lib_free(item_sizes);
}

static void monitor_binary_process_mem_get(binary_command_t *command)
{
    unsigned char *response;
    unsigned char *response_cursor;

    uint32_t response_size = 2;
    int banknum = 0;
    int old_sidefx = sidefx;
    MEMSPACE memspace = e_default_space;

    unsigned char *body = command->body;

    uint8_t new_sidefx = body[0];

    uint16_t startaddress = little_endian_to_uint16(&body[1]);
    uint16_t endaddress = little_endian_to_uint16(&body[3]);

    uint8_t requested_memspace = body[5];
    uint16_t requested_banknum = little_endian_to_uint16(&body[6]);

    uint16_t length = endaddress - startaddress + 1;

    if (startaddress > endaddress) {
        monitor_binary_error(e_MON_ERR_INVALID_PARAMETER, command->request_id);
        log_message(LOG_DEFAULT, "monitor binary memget: wrong start and/or end address %04x - %04x",
                    startaddress, endaddress);
        return;
    }

    if (command->length < 8) {
        monitor_binary_error(e_MON_ERR_CMD_INVALID_LENGTH, command->request_id);
        return;
    }

    if (requested_memspace == 0) {
        memspace = e_comp_space;
    } else if (requested_memspace == 1) {
        memspace = e_disk8_space;
    } else if (requested_memspace == 2) {
        memspace = e_disk9_space;
    } else if (requested_memspace == 3) {
        memspace = e_disk10_space;
    } else if (requested_memspace == 4) {
        memspace = e_disk11_space;
    } else {
        monitor_binary_error(e_MON_ERR_INVALID_PARAMETER, command->request_id);
        log_message(LOG_DEFAULT, "monitor binary memget: Unknown memspace %u", requested_memspace);
        return;
    }

    if (mon_banknum_validate(memspace, requested_banknum) == 0) {
        monitor_binary_error(e_MON_ERR_INVALID_PARAMETER, command->request_id);
        log_message(LOG_DEFAULT, "monitor binary memget: Unknown bank %u", requested_banknum);
        return;
    }

    banknum = requested_banknum;

    response_size += length;

    response = lib_malloc(response_size);
    response_cursor = response;

    response_cursor = write_uint16(length, response_cursor);

    sidefx = !!new_sidefx;
    mon_get_mem_block_ex(memspace, banknum, startaddress, endaddress - startaddress, response_cursor);
    sidefx = old_sidefx;

    response_cursor += length;

    monitor_binary_response(response_size, e_MON_RESPONSE_MEM_GET, e_MON_ERR_OK, command->request_id, response);

    lib_free(response);
}

static void monitor_binary_process_mem_set(binary_command_t *command)
{
    unsigned int i;

    int banknum = 0;
    const int header_size = 8;
    int old_sidefx = sidefx;
    MEMSPACE memspace = e_default_space;

    unsigned char *body = command->body;

    uint8_t new_sidefx = body[0];

    uint16_t startaddress = little_endian_to_uint16(&body[1]);
    uint16_t endaddress = little_endian_to_uint16(&body[3]);

    uint8_t requested_memspace = body[5];
    uint16_t requested_banknum = little_endian_to_uint16(&body[6]);

    uint16_t length = endaddress - startaddress + 1;

    if (startaddress > endaddress) {
        monitor_binary_error(e_MON_ERR_INVALID_PARAMETER, command->request_id);
        log_message(LOG_DEFAULT, "monitor binary memset: wrong start and/or end address %04x - %04x",
                    startaddress, endaddress);
        return;
    }

    if (command->length < length + header_size) {
        monitor_binary_error(e_MON_ERR_CMD_INVALID_LENGTH, command->request_id);
        return;
    }

    if (requested_memspace == 0) {
        memspace = e_comp_space;
    } else if (requested_memspace == 1) {
        memspace = e_disk8_space;
    } else if (requested_memspace == 2) {
        memspace = e_disk9_space;
    } else if (requested_memspace == 3) {
        memspace = e_disk10_space;
    } else if (requested_memspace == 4) {
        memspace = e_disk11_space;
    } else {
        monitor_binary_error(e_MON_ERR_INVALID_PARAMETER, command->request_id);
        log_message(LOG_DEFAULT, "monitor binary memset: Unknown memspace %u", requested_memspace);
        return;
    }

    if (mon_banknum_validate(memspace, requested_banknum) == 0) {
        monitor_binary_error(e_MON_ERR_INVALID_PARAMETER, command->request_id);
        log_message(LOG_DEFAULT, "monitor binary memset: Unknown bank %u", requested_banknum);
        return;
    }

    banknum = requested_banknum;

    sidefx = !!new_sidefx;
    for (i = 0; i < length; i++) {
        mon_set_mem_val_ex(memspace, banknum, (uint16_t)ADDR_LIMIT(startaddress + i), body[header_size + i]);
    }
    sidefx = old_sidefx;

    monitor_binary_response(0, e_MON_RESPONSE_MEM_SET, e_MON_ERR_OK, command->request_id, NULL);
}

static void monitor_binary_process_command(unsigned char * pbuffer)
{
    BINARY_COMMAND command_type;
    binary_command_t *command = lib_malloc(sizeof(binary_command_t));

    command->api_version = (uint8_t)pbuffer[1];

    if (command->api_version != 0x01) {
        monitor_binary_error(e_MON_ERR_INVALID_PARAMETER, command->request_id);
        return;
    }

    command->length = little_endian_to_uint32(&pbuffer[2]);

    if (command->api_version >= 0x01) {
        command->request_id = little_endian_to_uint32(&pbuffer[6]);
        command->type = pbuffer[10];
        command->body = &pbuffer[11];
    }

    command_type = command->type;
    if (command_type == e_MON_CMD_PING) {
        monitor_binary_process_ping(command);

    } else if (command_type == e_MON_CMD_MEM_GET) {
        monitor_binary_process_mem_get(command);
    } else if (command_type == e_MON_CMD_MEM_SET) {
        monitor_binary_process_mem_set(command);

    } else if (command_type == e_MON_CMD_CHECKPOINT_GET) {
        monitor_binary_process_checkpoint_get(command);
    } else if (command_type == e_MON_CMD_CHECKPOINT_SET) {
        monitor_binary_process_checkpoint_set(command);
    } else if (command_type == e_MON_CMD_CHECKPOINT_DELETE) {
        monitor_binary_process_checkpoint_delete(command);
    } else if (command_type == e_MON_CMD_CHECKPOINT_LIST) {
        monitor_binary_process_checkpoint_list(command);
    } else if (command_type == e_MON_CMD_CHECKPOINT_TOGGLE) {
        monitor_binary_process_checkpoint_toggle(command);

    } else if (command_type == e_MON_CMD_CONDITION_SET) {
        monitor_binary_process_condition_set(command);

    } else if (command_type == e_MON_CMD_REGISTERS_GET) {
        monitor_binary_process_registers_get(command);
    } else if (command_type == e_MON_CMD_REGISTERS_SET) {
        monitor_binary_process_registers_set(command);

    } else if (command_type == e_MON_CMD_ADVANCE_INSTRUCTIONS) {
        monitor_binary_process_advance_instructions(command);
    } else if (command_type == e_MON_CMD_KEYBOARD_FEED) {
        monitor_binary_process_keyboard_feed(command);
    } else if (command_type == e_MON_CMD_EXECUTE_UNTIL_RETURN) {
        monitor_binary_process_execute_until_return(command);

    } else if (command_type == e_MON_CMD_EXIT) {
        monitor_binary_process_exit(command);
    } else if (command_type == e_MON_CMD_QUIT) {
        monitor_binary_process_quit(command);
    } else if (command_type == e_MON_CMD_RESET) {
        monitor_binary_process_reset(command);
    } else if (command_type == e_MON_CMD_AUTOSTART) {
        monitor_binary_process_autostart(command);

    } else if (command_type == e_MON_CMD_BANKS_AVAILABLE) {
        monitor_binary_process_banks_available(command);
    } else if (command_type == e_MON_CMD_REGISTERS_AVAILABLE) {
        monitor_binary_process_registers_available(command);
    } else {
        monitor_binary_error(e_MON_ERR_INVALID_PARAMETER, command->request_id);
        log_message(LOG_DEFAULT,
                "monitor_network binary command: unknown command %u, "
                "skipping command length of %u",
                command->type, command->length);
    }

    pbuffer[0] = 0;

    lib_free(command);
}

static int monitor_binary_activate(void)
{
    vice_network_socket_address_t * server_addr = NULL;
    int error = 1;

    do {
        if (!monitor_binary_server_address) {
            break;
        }

        server_addr = vice_network_address_generate(monitor_binary_server_address, 0);
        if (!server_addr) {
            break;
        }

        listen_socket = vice_network_server(server_addr);
        if (!listen_socket) {
            break;
        }

        error = 0;
    } while (0);

    if (server_addr) {
        vice_network_address_close(server_addr);
    }

    return error;
}

int monitor_binary_get_command_line(void)
{
    static size_t buffer_size = 0;
    static unsigned char *buffer;

    while (monitor_binary_data_available()) {
        uint32_t body_length;
        uint8_t api_version;
        unsigned int remaining_header_size = 5;
        unsigned int command_size;

        if (!buffer) {
            buffer = lib_malloc(300);
        }

        int n = monitor_binary_receive(buffer, 1);
        if (n == 0) {
            monitor_binary_quit();
            return 0;
        } else if (n < 0) {
            monitor_binary_quit();
            return 0;
        }
        
        if (buffer[0] != ASC_STX) {
            continue;
        }

        n = monitor_binary_receive(&buffer[1], sizeof(api_version) + sizeof(body_length));

        if (n < sizeof(api_version) + sizeof(body_length)) {
            monitor_binary_quit();
            return 0;
        }

        api_version = buffer[1];
        body_length = little_endian_to_uint32(&buffer[2]);

        if (api_version == 0x01) {
            remaining_header_size = 5;
        } else {
            continue;
        }

        command_size = sizeof(api_version) + sizeof(body_length) + remaining_header_size + body_length + 1;
        if (!buffer || buffer_size < command_size) {
            buffer = lib_realloc(buffer, command_size);
            buffer_size = command_size;
        }

        n = 0;

        while (n < remaining_header_size + body_length) {
            int o = monitor_binary_receive(&buffer[6 + n], remaining_header_size + body_length - n);
            if (o <= 0) {
                monitor_binary_quit();
                return 0;
            }

            n += o;
        }

        monitor_binary_process_command(buffer);

        if (exit_mon) {
            return 0;
        }
    }

    return 1;
}

static int monitor_binary_deactivate(void)
{
    if (listen_socket) {
        vice_network_socket_close(listen_socket);
        listen_socket = NULL;
    }

    return 0;
}

/* ------------------------------------------------------------------------- */

/*! \internal \brief set the binary monitor to the enabled or disabled state

 \param val
   if 0, disable the binary monitor; else, enable it.

 \param param
   unused

 \return
   0 on success. else -1.
*/
static int set_binary_monitor_enabled(int value, void *param)
{
    int val = value ? 1 : 0;

    if (!val) {
        if (monitor_binary_enabled) {
            if (monitor_binary_deactivate() < 0) {
                return -1;
            }
        }
        monitor_binary_enabled = 0;
        return 0;
    } else {
        if (!monitor_binary_enabled) {
            if (monitor_binary_activate() < 0) {
                return -1;
            }
        }

        monitor_binary_enabled = 1;
        return 0;
    }
}

/*! \internal \brief set the network address of the network monitor

 \param name
   pointer to a buffer which holds the network server addresss.

 \param param
   unused

 \return
   0 on success, else -1.
*/
static int set_binary_server_address(const char *name, void *param)
{
    if (monitor_binary_server_address != NULL && name != NULL
        && strcmp(name, monitor_binary_server_address) == 0) {
        return 0;
    }

    if (monitor_binary_enabled) {
        monitor_binary_deactivate();
    }
    util_string_set(&monitor_binary_server_address, name);

    if (monitor_binary_enabled) {
        monitor_binary_activate();
    }

    return 0;
}

/*! \brief string resources used by the binary monitor module */
static const resource_string_t resources_string[] = {
    { "BinaryMonitorServerAddress", "ip4://127.0.0.1:6502", RES_EVENT_NO, NULL,
      &monitor_binary_server_address, set_binary_server_address, NULL },
    RESOURCE_STRING_LIST_END
};

/*! \brief integer resources used by the binary monitor module */
static const resource_int_t resources_int[] = {
    { "BinaryMonitorServer", 0, RES_EVENT_STRICT, (resource_value_t)0,
      &monitor_binary_enabled, set_binary_monitor_enabled, NULL },
    RESOURCE_INT_LIST_END
};

/*! \brief initialize the binary monitor resources
 \return
   0 on success, else -1.

 \remark
   Registers the string and the integer resources
*/
int monitor_binary_resources_init(void)
{
    if (resources_register_string(resources_string) < 0) {
        return -1;
    }

    return resources_register_int(resources_int);
}

/*! \brief uninitialize the network monitor resources */
void monitor_binary_resources_shutdown(void)
{
    monitor_binary_deactivate();
    monitor_binary_quit();

    lib_free(monitor_binary_server_address);
}

/* ------------------------------------------------------------------------- */

static const cmdline_option_t cmdline_options[] =
{
    { "-binarymonitor", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "BinaryMonitorServer", (resource_value_t)1,
      NULL, "Enable binary monitor" },
    { "+binarymonitor", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "BinaryMonitorServer", (resource_value_t)0,
      NULL, "Disable binary monitor" },
    { "-binarymonitoraddress", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "BinaryMonitorServerAddress", NULL,
      "<Name>", "The local address the binary monitor should bind to" },
    CMDLINE_LIST_END
};

/*! \brief initialize the command-line options'
 \return
   0 on success, else -1.

 \remark
   Registers the command-line options
*/
int monitor_binary_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}

int monitor_is_binary(void)
{
    return connected_socket != NULL;
}

#else

int monitor_binary_resources_init(void)
{
    return 0;
}

void monitor_binary_resources_shutdown(void)
{
}

int monitor_binary_cmdline_options_init(void)
{
    return 0;
}

void monitor_check_binary(void)
{
}

int monitor_binary_transmit(const unsigned char *buffer, size_t buffer_length)
{
    return 0;
}

int monitor_binary_get_command_line(void)
{
    return 0;
}

int monitor_is_binary(void)
{
    return 0;
}

void monitor_binary_event_opened(void) {
}

void monitor_binary_event_closed(void) {
}

ui_jam_action_t monitor_binary_ui_jam_dialog(const char *format, ...)
{
    return UI_JAM_HARD_RESET;
}

void monitor_binary_response_checkpoint_info(uint32_t request_id, mon_checkpoint_t *checkpt, bool hit) {
}

#endif
