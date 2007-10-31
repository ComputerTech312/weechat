/*
 * Copyright (c) 2003-2007 by FlashCode <flashcode@flashtux.org>
 * See README for License detail, AUTHORS for developers list.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* irc-display.c: display functions for IRC */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "../../core/weechat.h"
#include "irc.h"
#include "../../core/utf8.h"
#include "../../core/weechat-config.h"
#include "../../gui/gui.h"


/*
 * irc_display_hide_password: hide IRC password(s) in a string
 */

void
irc_display_hide_password (char *string, int look_for_nickserv)
{
    char *pos_nickserv, *pos, *pos_pwd;

    pos = string;
    while (1)
    {
        if (look_for_nickserv)
        {
            pos_nickserv = strstr (pos, "nickserv ");
            if (!pos_nickserv)
                return;
            pos = pos_nickserv + 9;
            while (pos[0] == ' ')
                pos++;
            if ((strncmp (pos, "identify ", 9) == 0)
                || (strncmp (pos, "register ", 9) == 0))
                pos_pwd = pos + 9;
            else
                pos_pwd = NULL;
        }
        else
        {
            pos_pwd = strstr (pos, "identify ");
            if (!pos_pwd)
                pos_pwd = strstr (pos, "register ");
            if (!pos_pwd)
                return;
            pos_pwd += 9;
        }

        if (pos_pwd)
        {
            while (pos_pwd[0] == ' ')
                pos_pwd++;
            
            while (pos_pwd[0] && (pos_pwd[0] != ';') && (pos_pwd[0] != ' ')
                   && (pos_pwd[0] != '"'))
            {
                pos_pwd[0] = '*';
                pos_pwd++;
            }
            pos = pos_pwd;
        }
    }
}

/*
 * irc_display_nick: display nick in chat window
 */

void
irc_display_nick (t_gui_buffer *buffer, t_irc_nick *nick, char *nickname,
                  int type, int display_around, char *force_color, int no_nickmode)
{
    char format[32], *ptr_nickname;
    t_irc_server *ptr_server;
    t_irc_channel *ptr_channel;
    int is_private, max_align, i, nickname_length, external_nick;
    int length, spaces, disable_prefix_suffix;
    
    max_align = (cfg_look_align_size_max >= cfg_look_align_size) ?
        cfg_look_align_size_max : cfg_look_align_size;

    ptr_server = irc_server_search (buffer->category);
    ptr_channel = irc_channel_search (ptr_server, buffer->name);
    is_private = (ptr_channel && (ptr_channel->type != IRC_CHANNEL_TYPE_CHANNEL));
    
    ptr_nickname = strdup ((nick) ? nick->nick : nickname);
    if (!ptr_nickname)
        return;
    nickname_length = utf8_width_screen (ptr_nickname); 
    external_nick = (!nick && !is_private);
    disable_prefix_suffix = ((cfg_look_align_nick != CFG_LOOK_ALIGN_NICK_NONE)
                             && ((int)strlen (cfg_look_nick_prefix) +
                                 (int)strlen (cfg_look_nick_suffix) > max_align - 4));
    
    /* calculate length to display, to truncate it if too long */
    length = nickname_length;
    if (!disable_prefix_suffix && cfg_look_nick_prefix)
        length += strlen (cfg_look_nick_prefix);
    if (external_nick)
        length += 2;
    if (nick && cfg_look_nickmode)
    {
        if (nick->flags & (IRC_NICK_CHANOWNER | IRC_NICK_CHANADMIN |
                           IRC_NICK_CHANADMIN2 | IRC_NICK_OP | IRC_NICK_HALFOP |
                           IRC_NICK_VOICE | IRC_NICK_CHANUSER))
            length += 1;
        else if (cfg_look_nickmode_empty && !no_nickmode)
            length += 1;
    }
    if (!disable_prefix_suffix && cfg_look_nick_suffix)
        length += strlen (cfg_look_nick_suffix);
    
    /* calculate number of spaces to insert before or after nick */
    spaces = 0;
    if (cfg_look_align_nick != CFG_LOOK_ALIGN_NICK_NONE)
    {
        if (length > max_align)
            spaces = max_align - length;
        else if (length > cfg_look_align_size)
            spaces = 0;
        else
            spaces = cfg_look_align_size - length;
    }
    
    /* display prefix */
    if (display_around && !disable_prefix_suffix
        && cfg_look_nick_prefix && cfg_look_nick_prefix[0])
        gui_chat_printf_type (buffer, type, NULL, -1,
                              "%s%s",
                              GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                              cfg_look_nick_prefix);
    
    /* display spaces before nick, if needed */
    if (display_around
        && (cfg_look_align_nick == CFG_LOOK_ALIGN_NICK_RIGHT)
        && (spaces > 0))
    {
        snprintf (format, 32, "%%-%ds", spaces);
        gui_chat_printf_type (buffer, type, NULL, -1, format, " ");
    }
    
    /* display nick mode */
    if (nick && cfg_look_nickmode)
    {
        if (nick->flags & IRC_NICK_CHANOWNER)
            gui_chat_printf_type (buffer, type, NULL, -1, "%s~",
                                  GUI_COLOR(GUI_COLOR_NICKLIST_PREFIX1));
        else if (nick->flags & IRC_NICK_CHANADMIN)
            gui_chat_printf_type (buffer, type, NULL, -1, "%s&",
                                  GUI_COLOR(GUI_COLOR_NICKLIST_PREFIX1));
        else if (nick->flags & IRC_NICK_CHANADMIN2)
            gui_chat_printf_type (buffer, type, NULL, -1, "%s!",
                                  GUI_COLOR(GUI_COLOR_NICKLIST_PREFIX1));
        else if (nick->flags & IRC_NICK_OP)
            gui_chat_printf_type (buffer, type, NULL, -1, "%s@",
                                  GUI_COLOR(GUI_COLOR_NICKLIST_PREFIX1));
        else if (nick->flags & IRC_NICK_HALFOP)
            gui_chat_printf_type (buffer, type, NULL, -1, "%s%%",
                                  GUI_COLOR(GUI_COLOR_NICKLIST_PREFIX2));
        else if (nick->flags & IRC_NICK_VOICE)
            gui_chat_printf_type (buffer, type, NULL, -1, "%s+",
                                  GUI_COLOR(GUI_COLOR_NICKLIST_PREFIX3));
        else if (nick->flags & IRC_NICK_CHANUSER)
            gui_chat_printf_type (buffer, type, NULL, -1, "%s-",
                                  GUI_COLOR(GUI_COLOR_NICKLIST_PREFIX4));
        else if (cfg_look_nickmode_empty && !no_nickmode)
            gui_chat_printf_type (buffer, type, NULL, -1, "%s ",
                                  GUI_COLOR(GUI_COLOR_CHAT));
    }
    
    /* display nick */
    if (external_nick)
        gui_chat_printf_type (buffer, type, NULL, -1, "%s%s",
                              GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                              "(");
    if (display_around && (spaces < 0))
    {
        i = nickname_length + spaces - 1;
        if (i < 3)
        {
            if (nickname_length < 3)
                i = nickname_length;
            else
                i = 3;
        }
        ptr_nickname[i] = '\0';
    }
    if (display_around)
        gui_chat_printf_type_nick (buffer, type,
                                   (nick) ? nick->nick : nickname,
                                   NULL, -1,
                                   "%s%s",
                                   (force_color) ? force_color :
                                   GUI_COLOR((nick) ?
                                             nick->color : cfg_col_chat),
                                   ptr_nickname);
    else
        gui_chat_printf_type (buffer, type, NULL, -1,
                              "%s%s",
                              (force_color) ? force_color :
                              GUI_COLOR((nick) ? nick->color : cfg_col_chat),
                              ptr_nickname);
    if (display_around && (spaces < 0))
        gui_chat_printf_type (buffer, type, NULL, -1, "%s+",
                              GUI_COLOR(GUI_COLOR_NICKLIST_MORE));
    if (external_nick)
        gui_chat_printf_type (buffer, type, NULL, -1, "%s%s",
                              GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                         ")");
    
    /* display spaces after nick, if needed */
    if (display_around
        && (cfg_look_align_nick == CFG_LOOK_ALIGN_NICK_LEFT)
        && (spaces > 0))
    {
        snprintf (format, 32, "%%-%ds", spaces);
        gui_chat_printf_type (buffer, type, NULL, -1, format, " ");
    }
    
    /* display suffix */
    if (display_around && !disable_prefix_suffix
        && cfg_look_nick_suffix && cfg_look_nick_suffix[0])
        gui_chat_printf_type (buffer, type, NULL, -1, "%s%s",
                              GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                              cfg_look_nick_suffix);
    
    gui_chat_printf_type (buffer, type, NULL, -1, "%s%s",
                          GUI_NO_COLOR,
                          (display_around) ? " " : "");
    free (ptr_nickname);
}

/*
 * irc_display_away: display away on all channels of all servers
 */

void
irc_display_away (t_irc_server *server, char *string1, char *string2)
{
    t_irc_channel *ptr_channel;
    char format[32];
    
    for (ptr_channel = server->channels; ptr_channel;
         ptr_channel = ptr_channel->next_channel)
    {
        if (ptr_channel->type == IRC_CHANNEL_TYPE_CHANNEL)
        {
            if (cfg_look_align_other)
            {
                snprintf (format, 32, "%%-%ds", cfg_look_align_size + 1);
                gui_chat_printf_type (ptr_channel->buffer, GUI_MSG_TYPE_NICK,
                                      NULL, -1,
                                      format, " ");
            }
            gui_chat_printf_nolog (ptr_channel->buffer,
                                   NULL, -1,
                                   "%s[%s%s%s %s: %s%s]\n",
                                   GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                                   GUI_COLOR(GUI_COLOR_CHAT_NICK),
                                   server->nick,
                                   GUI_COLOR(GUI_COLOR_CHAT),
                                   string1,
                                   string2,
                                   GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS));
        }
    }
}

/*
 * irc_display_mode: display IRC message for mode change
 */

void
irc_display_mode (t_gui_buffer *buffer,
                  char *channel_name, char *nick_name, char set_flag,
                  char *symbol, char *nick_host, char *message, char *param)
{
    gui_chat_printf_info (buffer,
                          "%s[%s%s%s/%s%c%s%s] %s%s",
                          GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                          (channel_name) ?
                          GUI_COLOR(GUI_COLOR_CHAT_CHANNEL) :
                          GUI_COLOR(GUI_COLOR_CHAT_NICK),
                          (channel_name) ? channel_name : nick_name,
                          GUI_COLOR(GUI_COLOR_CHAT),
                          GUI_COLOR(GUI_COLOR_CHAT_CHANNEL),
                          set_flag,
                          symbol,
                          GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                          GUI_COLOR(GUI_COLOR_CHAT_NICK),
                          nick_host);
    if (param)
        gui_chat_printf (buffer, " %s%s %s%s\n",
                         GUI_COLOR(GUI_COLOR_CHAT),
                         message,
                         GUI_COLOR(GUI_COLOR_CHAT_NICK),
                         param);
    else
        gui_chat_printf (buffer, " %s%s\n",
                         GUI_COLOR(GUI_COLOR_CHAT),
                         message);
}

/*
 * irc_display_server: display server description
 */

void
irc_display_server (t_irc_server *server, int with_detail)
{
    char *string;
    int num_channels, num_pv;
    
    if (with_detail)
    {
        gui_chat_printf (NULL, "\n");
        gui_chat_printf (NULL, _("%sServer: %s%s %s[%s%s%s]\n"),
                         GUI_COLOR(GUI_COLOR_CHAT),
                         GUI_COLOR(GUI_COLOR_CHAT_SERVER),
                         server->name,
                         GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                         GUI_COLOR(GUI_COLOR_CHAT),
                         (server->is_connected) ?
                         _("connected") : _("not connected"),
                         GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS));
        
        gui_chat_printf (NULL, "  server_autoconnect . . . . : %s%s\n",
                         (server->autoconnect) ? _("on") : _("off"),
                         (server->temp_server) ?
                         _(" (temporary server, will not be saved)") : "");
        gui_chat_printf (NULL, "  server_autoreconnect . . . : %s\n",
                         (server->autoreconnect) ? _("on") : _("off"));
        gui_chat_printf (NULL, "  server_autoreconnect_delay : %d %s\n",
                         server->autoreconnect_delay,
                         _("seconds"));
        gui_chat_printf (NULL, "  server_address . . . . . . : %s\n",
                         server->address);
        gui_chat_printf (NULL, "  server_port  . . . . . . . : %d\n",
                         server->port);
        gui_chat_printf (NULL, "  server_ipv6  . . . . . . . : %s\n",
                         (server->ipv6) ? _("on") : _("off"));
        gui_chat_printf (NULL, "  server_ssl . . . . . . . . : %s\n",
                         (server->ssl) ? _("on") : _("off"));
        gui_chat_printf (NULL, "  server_password  . . . . . : %s\n",
                         (server->password && server->password[0]) ?
                         _("(hidden)") : "");
        gui_chat_printf (NULL, "  server_nick1/2/3 . . . . . : "
                         "%s %s/ %s%s %s/ %s%s\n",
                         server->nick1,
                         GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                         GUI_COLOR(GUI_COLOR_CHAT),
                         server->nick2,
                         GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                         GUI_COLOR(GUI_COLOR_CHAT),
                         server->nick3);
        gui_chat_printf (NULL, "  server_username  . . . . . : %s\n",
                         server->username);
        gui_chat_printf (NULL, "  server_realname  . . . . . : %s\n",
                         server->realname);
        gui_chat_printf (NULL, "  server_hostname  . . . . . : %s\n",
                         (server->hostname) ? server->hostname : "");
        if (server->command && server->command[0])
            string = strdup (server->command);
        else
            string = NULL;
        if (string)
        {
            if (irc_cfg_log_hide_nickserv_pwd)
                irc_display_hide_password (string, 1);
            gui_chat_printf (NULL, "  server_command . . . . . . : %s\n",
                             string);
            free (string);
        }
        else
            gui_chat_printf (NULL, "  server_command . . . . . . : %s\n",
                             (server->command && server->command[0]) ?
                             server->command : "");
        gui_chat_printf (NULL, "  server_command_delay . . . : %d %s\n",
                         server->command_delay,
                         _("seconds"));
        gui_chat_printf (NULL, "  server_autojoin  . . . . . : %s\n",
                         (server->autojoin && server->autojoin[0]) ?
                         server->autojoin : "");
        gui_chat_printf (NULL, "  server_notify_levels . . . : %s\n",
                         (server->notify_levels && server->notify_levels[0]) ?
                         server->notify_levels : "");
    }
    else
    {
        gui_chat_printf (NULL, " %s %s%s ",
                         (server->is_connected) ? "*" : " ",
                         GUI_COLOR(GUI_COLOR_CHAT_SERVER),
                         server->name);
        gui_chat_printf (NULL, "%s[%s%s",
                         GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                         GUI_COLOR(GUI_COLOR_CHAT),
                         (server->is_connected) ?
                         _("connected") : _("not connected"));
        if (server->is_connected)
        {
            num_channels = irc_server_get_channel_count (server);
            num_pv = irc_server_get_pv_count (server);
            gui_chat_printf (NULL, ", ");
            gui_chat_printf (NULL,
                             NG_("%d channel", "%d channels",
                                 num_channels),
                             num_channels);
            gui_chat_printf (NULL, ", ");
            gui_chat_printf (NULL, _("%d pv"), num_pv);
        }
        gui_chat_printf (NULL, "%s]%s%s\n",
                         GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS),
                         GUI_COLOR(GUI_COLOR_CHAT),
                         (server->temp_server) ? _(" (temporary)") : "");
    }
}
