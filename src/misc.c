/******************************************************************************
*   TinTin++                                                                  *
*   Copyright (C) 2004 (See CREDITS file)                                     *
*                                                                             *
*   This program is protected under the GNU GPL (See COPYING)                 *
*                                                                             *
*   This program is free software; you can redistribute it and/or modify      *
*   it under the terms of the GNU General Public License as published by      *
*   the Free Software Foundation; either version 2 of the License, or         *
*   (at your option) any later version.                                       *
*                                                                             *
*   This program is distributed in the hope that it will be useful,           *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of            *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
*   GNU General Public License for more details.                              *
*                                                                             *
*   You should have received a copy of the GNU General Public License         *
*   along with this program; if not, write to the Free Software               *
*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA *
*******************************************************************************/

/******************************************************************************
*                (T)he K(I)cki(N) (T)ickin D(I)kumud Clie(N)t                 *
*                                                                             *
*                         coded by Peter Unold 1992                           *
******************************************************************************/

#include "tintin.h"


DO_COMMAND(do_all)
{
	char *left;
	struct session *sesptr, *next_ses;

	if (gts->next)
	{
		get_arg_in_braces(arg, &left, TRUE);

		for (sesptr = gts->next ; sesptr ; sesptr = next_ses)
		{
			next_ses = sesptr->next;
			parse_input(sesptr, left);
		}
	}
	else
	{
		tintin_puts2(ses, "#BUT THERE AREN'T ANY SESSIONS AT ALL!");
	}
	return ses;
}


DO_COMMAND(do_bell)
{
	printf("\007");

	return ses;
}


DO_COMMAND(do_commands)
{
	char buf[BUFFER_SIZE] = { 0 }, add[BUFFER_SIZE];
	int cmd;

	tintin_header(ses, " %s ", "COMMANDS");

	for (cmd = 0 ; *command_table[cmd].name != 0 ; cmd++)
	{
		if (!is_abbrev(arg, command_table[cmd].name))
		{
			continue;
		}
		if (strlen(buf) + 20 > ses->cols)
		{
			tintin_puts2(ses, buf);
			buf[0] = 0;
		}
		sprintf(add, "%20s", command_table[cmd].name);
		strcat(buf, add);
	}
	if (buf[0])
	{
		tintin_puts2(ses, buf);
	}
	tintin_header(ses, "");

	return ses;
}


DO_COMMAND(do_cr)
{
	write_line_mud("\r\n", ses);

	return ses;
}


DO_COMMAND(do_echo)
{
	char *temp;

	temp = stringf_alloc("result %s", arg);

	internal_variable(ses, "{result}");

	do_format(ses, temp);

	substitute(ses, "$result", &temp, SUB_VAR);

	if (*temp)
	{
		do_showme(ses, temp);
	}

	return ses;
}


DO_COMMAND(do_end)
{
	if (*arg)
	{
		quitmsg(arg);
	}
	else
	{
		quitmsg(NULL);
	}
	return NULL;
}


DO_COMMAND(do_forall)
{
	char *left, *right, *temp;

	arg = get_arg_in_braces(arg, &temp,  TRUE);
	arg = get_arg_in_braces(arg, &right, TRUE);

	substitute(ses, temp, &left, SUB_VAR|SUB_FUN);

	if (*left == 0 || *right == 0)
	{
		tintin_printf2(ses, "#ERROR: #FORALL - PROVIDE 2 ARGUMENTS");
	}
	else
	{
		arg = left;

		while (*arg)
		{
			arg = get_arg_in_braces(arg, &gtd->cmds[0], FALSE);

			internal_variable(ses, "{forall} {%s}", gtd->cmds[0]);

			substitute(ses, right, &temp, SUB_CMD);

			ses = parse_input(ses, temp);
		}
	}
	return ses;
}


DO_COMMAND(do_gagline)
{
	SET_BIT(ses->flags, SES_FLAG_GAG);

	return ses;
}


DO_COMMAND(do_info)
{
	int cnt;

	if (*arg == 'c')
	{
		show_cpu(ses);

		return ses;
	}

	tintin_header(ses, " INFORMATION ");

	for (cnt = 0 ; cnt < LIST_MAX ; cnt++)
	{
		if (!HAS_BIT(ses->list[cnt]->flags, LIST_FLAG_SHOW))
		{
			continue;
		}
		tintin_printf2(ses, "%-20s  %5d  IGNORE %3s  MESSAGE %3s  DEBUG %3s",
			list_table[cnt].name_multi,
			ses->list[cnt]->count,
			HAS_BIT(ses->list[cnt]->flags, LIST_FLAG_IGNORE) ? "ON" : "OFF",
			HAS_BIT(ses->list[cnt]->flags, LIST_FLAG_MESSAGE) ? "ON" : "OFF",
			HAS_BIT(ses->list[cnt]->flags, LIST_FLAG_DEBUG)   ? "ON" : "OFF");
	}
	tintin_header(ses, "");

	return ses;
}


DO_COMMAND(do_loop)
{
	char *left, *right, *temp, *command;

	long long bound1, bound2, counter, step;

	arg = get_arg_in_braces(arg, &temp, FALSE);
	arg = get_arg_in_braces(arg, &command, TRUE);

	arg = get_arg_in_braces(temp, &left, FALSE);
	arg = get_arg_in_braces(arg, &right, FALSE);


	if (*command == 0 || (*left == 0 && *right == 0))
	{
		show_message(ses, LIST_MATH, "#LOOP: INVALID ARGUMENTS.");

		return ses;
	}

	if (*left && *right)
	{
		bound1 = (long long) get_number(ses, left);
		bound2 = (long long) get_number(ses, right);

		if (bound1 <= bound2)
		{
			step = 1;
			bound2++;
		}
		else
		{
			step = -1;
			bound2--;
		}

		for (counter = bound1 ; counter != bound2 ; counter += step)
		{
			gtd->cmds[0] = stringf_alloc("%lld", counter);

			internal_variable(ses, "{loop} {%lld}", counter);

			substitute(ses, command, &temp, SUB_CMD);
			ses = parse_input(ses, temp);
		}
	}
	else
	{
		while ((long long) get_number(ses, left))
		{
			ses = parse_input(ses, command);
		}
	}
	return ses;
}


DO_COMMAND(do_nop)
{
	return ses;
}


DO_COMMAND(do_parse)
{
	char *left, *right, *temp;
	int cnt;

	arg = get_arg_in_braces(arg, &temp, 0);
	substitute(ses, temp, &left, SUB_VAR|SUB_FUN);

	arg = get_arg_in_braces(arg, &right, 1);

	if (*left == 0 || *right == 0)
	{
		tintin_printf2(ses, "#ERROR: #PARSE - PROVIDE 2 ARGUMENTS");
	}
	else
	{
		for (cnt = 0 ; left[cnt] != 0 ; cnt++)
		{
#ifdef BIG5
			if (left[cnt] & 0x80 && left[cnt+1] != 0)
			{
				gtd->cmds[0] = stringf_alloc("%c%c", left[cnt], left[++cnt]);
			}
			else
			{
				gtd->cmds[0] = stringf_alloc("%c", left[cnt]);
			}
#else
			gtd->cmds[0] = stringf_alloc("%c", left[cnt]);
#endif
			internal_variable(ses, "{parse} {%s}", gtd->cmds[0]);

			substitute(ses, right, &temp, SUB_CMD);

			ses = parse_input(ses, temp);
		}
	}
	return ses;
}


DO_COMMAND(do_return)
{
	SET_BIT(ses->flags, SES_FLAG_BREAK);

	if (*arg)
	{
		internal_variable(ses, "{result} {%s}", arg);
	}
	return ses;
}

DO_COMMAND(do_send)
{
	char *left, *out;

	get_arg_in_braces(arg, &left, TRUE);

	substitute(ses, left, &out, SUB_VAR|SUB_FUN|SUB_ESC|SUB_EOL);

	write_line_mud(out, ses);

	return ses;
}

DO_COMMAND(do_showme)
{
	char *left, *right, *temp, *output;

	arg = get_arg_in_braces(arg, &temp, TRUE);
	arg = get_arg_in_braces(arg, &right, FALSE);

	substitute(ses, temp, &left, SUB_COL|SUB_ESC);

	if (*right)
	{
		do_one_prompt(ses, left, atoi(right));

		return ses;
	}
	do_one_line(&left, ses);

	substitute(ses, left, &temp, SUB_COL|SUB_ESC);

	if (HAS_BIT(ses->flags, SES_FLAG_GAG))
	{
		DEL_BIT(ses->flags, SES_FLAG_GAG);

		return ses;
	}

	if (strip_vt102_strlen(ses->more_output) != 0)
	{
		output = stringf_alloc("\r\n\033[0m%s\033[0m", temp);
	}
	else
	{
		output = stringf_alloc("\033[0m%s\033[0m", temp);
	}

	add_line_buffer(ses, output, FALSE);

	if (ses != gtd->ses)
	{
		return ses;
	}

	if (!HAS_BIT(ses->flags, SES_FLAG_READMUD) && IS_SPLIT(ses))
	{
		save_pos(ses);
		goto_rowcol(ses, ses->bot_row, 1);
	}

	printline(ses, output, FALSE);

	if (!HAS_BIT(ses->flags, SES_FLAG_READMUD) && IS_SPLIT(ses))
	{
		restore_pos(ses);
	}

	return ses;
}

DO_COMMAND(do_snoop)
{
	struct session *sesptr = ses;
	char *left;

	get_arg_in_braces(arg, &left, 1);

	if (*left)
	{
		for (sesptr = gts->next ; sesptr ; sesptr = sesptr->next)
  		{
  			if (!strcmp(sesptr->name, left))
  			{
  				break;
  			}
  		}
		if (sesptr == NULL)
		{
			tintin_puts2(ses, "#NO SESSION WITH THAT NAME!");

			return ses;
		}
	}
	else
	{
		sesptr = ses;
	}

	if (HAS_BIT(sesptr->flags, SES_FLAG_SNOOP))
	{
		tintin_printf2(ses, "#UNSNOOPING SESSION '%s'", sesptr->name);
	}
	else
	{
		tintin_printf2(ses, "#SNOOPING SESSION '%s'", sesptr->name);
	}
	TOG_BIT(sesptr->flags, SES_FLAG_SNOOP);

	return ses;
}

DO_COMMAND(do_suspend)
{
	suspend_handler(0);

	return ses;
}


DO_COMMAND(do_zap)
{
	tintin_puts(ses, "");

	tintin_puts(ses, "#ZZZZZZZAAAAAAAAPPPP!!!!!!!!! LET'S GET OUTTA HERE!!!!!!!!");

	if (ses == gts)
	{
		return do_end(NULL, "");
	}
	cleanup_session(ses);

	return gtd->ses;
}

