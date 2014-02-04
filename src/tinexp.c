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
*   file: regexp.c - funtions related to tintin's regexp                      *
*           (T)he K(I)cki(N) (T)ickin D(I)kumud Clie(N)t ++ 2.00              *
*                        coded by peter unold 1992                            *
*                   recoded by Igor van den Hoven 2004                        *
******************************************************************************/

#include "tintin.h"


/******************************************************************************
* Simple regexp that skips vt102 codes and supports the * and ? wildcards     *
******************************************************************************/

int regexp(const char *exp, const char *str)
{
	short cnt;

	for ( ; *exp != 0 ; exp++, str++)
	{
		while ((cnt = skip_vt102_codes(str)) != 0)
		{
			str += cnt;
		}

#ifdef BIG5
		if (*exp & 0x80 && *str & 0x80)
		{
			if (*++exp != *++str)
			{
				return FALSE;
			}
			continue;
		}
#endif

		switch (*exp)
		{
			case '?':
				if (*str == 0)
				{
					return FALSE;
				}
				break;

			case '*':
				for (cnt = strlen(str) ; cnt >= 0 ; cnt--)
				{
					if (regexp(exp + 1, &str[cnt]))
					{
						return TRUE;
					}
				}
				return FALSE;

			case '\\':
				if (*++exp != *str)
				{
					return FALSE;
				}
				break;

			default:

				if (*exp != *str)
				{
					return FALSE;
				}

				break;
		}
	}
	return (*exp == *str);
}

/******************************************************************************
* copy *string into *result, but substitute the various expressions with the  *
* values they stand for.                                                      *
******************************************************************************/

void substitute(struct session *ses, const char *string, char *result, int flags)
{
	struct listnode *node;
	char temp[BUFFER_SIZE], buf[BUFFER_SIZE], *pti, *pto, *ptt;
	const char *pte;
	int i, cnt, flags_neol = flags;

	pti = (char *) string;
	pto = (char *) result;

	DEL_BIT(flags_neol, SUB_EOL);

	while (TRUE)
	{

#ifdef BIG5
		if (*pti & 0x80)
		{
			*pto++ = *pti++;
			if (*pti)
			{
				*pto++ = *pti++;
			}
			continue;
		}
#endif

		switch (*pti)
		{
			case '\0':
				if (HAS_BIT(flags, SUB_EOL))
				{
					*pto++ = '\r';
					*pto++ = '\n';
				}
				*pto = 0;
				return;

			case '$':
				if (HAS_BIT(flags, SUB_VAR) && (pti[1] == DEFAULT_OPEN || isalnum(pti[1])))
				{
					if (pti[1] == DEFAULT_OPEN)
					{
						for (ptt = buf, i = 2 ; pti[i] && pti[i] != DEFAULT_CLOSE ; i++)
						{
							*ptt++ = pti[i];
						}

						if (pti[i] == DEFAULT_CLOSE)
						{
							*ptt = 0;
							substitute(ses, buf, temp, flags_neol);
						}
						else
						{
							temp[0] = 0;
						}
					}

					if (isalnum(pti[1]))
					{
						for (ptt = temp, i = 1 ; isalnum(pti[i]) ; i++)
						{
							*ptt++ = pti[i];
						}
						*ptt = 0;
					}

					if ((node = searchnode_list(ses->list[LIST_VARIABLE], temp)) != NULL)
					{
						substitute(ses, node->right, pto, flags_neol - SUB_VAR);

						if (pti[1] == DEFAULT_OPEN)
						{
							pto += strlen(pto);
							pti += 3 + strlen(buf);
						}
						else
						{
							pto += strlen(pto);
							pti += 1 + strlen(temp);
						}
					}
					else
					{
						*pto++ = *pti++;
					}
				}
				else
				{
					*pto++ = *pti++;
				}
				break;

			case '<':
				if (HAS_BIT(flags, SUB_COL) && isdigit(pti[1]) && isdigit(pti[2]) && isdigit(pti[3]) && pti[4] == '>')
				{
					if (pti[1] != '8' || pti[2] != '8' || pti[3] != '8')
					{
						*pto++ = '\033';
						*pto++ = '[';

						switch (pti[1])
						{
							case '2':
								*pto++ = '2';
								*pto++ = '2';
								*pto++ = ';';
								break;
							case '8':
								break;
							default:
								*pto++ = pti[1];
								*pto++ = ';';
						}
						switch (pti[2])
						{
							case '8':
								break;
							default:
								*pto++ = '3';
								*pto++ = pti[2];
								*pto++ = ';';
								break;
						}
						switch (pti[3])
						{
							case '8':
								break;
							default:
								*pto++ = '4';
								*pto++ = pti[3];
								*pto++ = ';';
								break;
						}
						pto--;
						*pto++ = 'm';
					}
					pti += 5;
				}
				else
				{
					*pto++ = *pti++;
				}
				break;

			case '@':
				if (HAS_BIT(flags, SUB_FUN))
				{
					for (ptt = temp, i = 1 ; isalnum(pti[i]) ; i++)
					{
						*ptt++ = pti[i];
					}
					*ptt = 0;

					node = searchnode_list(ses->list[LIST_FUNCTION], temp);

					if (node == NULL || pti[i] != DEFAULT_OPEN)
					{
						*pto++ = *pti++;

						continue;
					}

					pti = (char *) get_arg_in_braces(&pti[i], temp, FALSE);

					substitute(ses, temp, buf, flags_neol);

					pte = buf;

					if (HAS_BIT(ses->list[LIST_FUNCTION]->flags, LIST_FLAG_DEBUG))
					{
						tintin_printf2(ses, "#FUNCTION DEBUG: (%s) (%s)", node->left, temp);
					}
					for (i = 0 ; i < 10 ; i++)
					{
						pte = get_arg_in_braces(pte, gtd->cmds[i], FALSE);
					}
					substitute(ses, node->right, temp, SUB_CMD);

					parse_input(temp, ses);

					DEL_BIT(ses->flags, SES_FLAG_BREAK);

					substitute(ses, "$result", pto, SUB_VAR);

					if (HAS_BIT(ses->list[LIST_FUNCTION]->flags, LIST_FLAG_DEBUG))
					{
						tintin_printf2(ses, "#FUNCTION DEBUG: (%s) (%s)", node->left, pto);
					}
					pto += strlen(pto);
				}
				else
				{
					*pto++ = *pti++;
				}
				break;

			case '%':
				if (HAS_BIT(flags, SUB_ARG) && isdigit(pti[1]))
				{
					ptt = gtd->vars[pti[1] - '0'];

					while (*ptt)
					{
#ifdef BIG5
						if (*ptt & 0x80)
						{
							*pto++ = *ptt++;
							if (*ptt)
							{
								*pto++ = *ptt++;
							}
							continue;
						}
#endif
						if (HAS_BIT(flags, SUB_SEC))
						{
							switch (*ptt)
							{
								case ';':
									*pto++ = '\\';
									*pto++ = ';';
									break;

								case '\\':
									*pto++ = '\\';
									*pto++ = '\\';
									break;

								default:
									*pto++ = *ptt;
									break;
							}
							ptt++;
						}
						else
						{
							*pto++ = *ptt++;
						}
					}
					pti += 2;
				}
				else if (HAS_BIT(flags, SUB_FMT))
				{
					*pto++ = '%';
					*pto++ = *pti++;
				}
				else
				{
					*pto++ = *pti++;
				}
				break;

			case '^':
				if (HAS_BIT(flags, SUB_ANC) && (pti == string || pti[1] == 0))
				{
					pti++;
				}
				else
				{
					*pto++ = *pti++;
				}
				break;

			case '~':
				if (HAS_BIT(flags, SUB_ANC) && pti == string)
				{
					string++;
					pti++;
				}
				else
				{
					*pto++ = *pti++;
				}
				break;

			case '&':
				if (HAS_BIT(flags, SUB_CMD) && isdigit(pti[1]))
				{
					for (cnt = 0 ; gtd->cmds[pti[1] - '0'][cnt] ; cnt++)
					{
						*pto++ = gtd->cmds[pti[1] - '0'][cnt];
					}
					pti += 2;
				}
				else
				{
					*pto++ = *pti++;
				}
				break;

			case '\\':
				if (HAS_BIT(flags, SUB_ESC))
				{
					pti++;

					switch (*pti)
					{
						case 'a':
							*pto++ = '\a';
							break;
						case 'b':
							*pto++ = '\b';
							break;
						case 'e':
							*pto++ = '\033';
							break;
						case 'n':
							*pto++ = '\n';
							break;
						case 'r':
							*pto++ = '\r';
							break;
						case 't':
							*pto++ = '\t';
							break;
						case 'x':
							pti++;

							if (pti[0] && pti[1])
							{
								*pto++ = hex_number(pti);
								pti += 2;
							}
							break;
						case '[':
							*pto++ = '{';
							break;
						case ']':
							*pto++ = '}';
							break;
						case '\0':
							*pto = 0;
							return;
						default:
							*pto++ = *pti;
							break;
					}
					pti++;
				}
				else
				{
					*pto++ = *pti++;
				}
				break;

			default:
				*pto++ = *pti++;
				break;
		}
	}
}


/******************************************************************************
* Calls action_regexp checking if the string matches, and automatically fills *
* in the text represented by the wildcards on success. Per default the entire *
* string is compared unless it starts or ends with the ^ character.           *
******************************************************************************/

int check_one_action(const char *line, const char *original, const char *action, struct session *ses)
{
	char result[BUFFER_SIZE], *ptr;
	const char *ptl;

	substitute(ses, action, result, SUB_VAR|SUB_FUN|SUB_ESC);

	ptr = result;

	if (*ptr == '~')
	{
		ptr++;
		ptl = original;
	}
	else
	{
		ptl = line;
	}

	if (*ptr == '^')
	{
		ptr++;
		return action_regexp(ptr, ptl);
	}

	if (ptr[0] == '%' && isdigit(ptr[1]))
	{
		return action_regexp(ptr, ptl);
	}

	while (*ptl)
	{
		if (*ptr == *ptl && action_regexp(ptr, ptl))
		{
			return TRUE;
		}
		ptl++;
	}
	return FALSE;
}


int action_regexp(const char *exp, const char *str)
{
	short cnt;

	while (*exp)
	{
#ifdef BIG5
		if (*exp & 0x80 && *str & 0x80)
		{
			exp++;
			str++;
			if (*exp++ != *str++)
			{
				return FALSE;
			}
			continue;
		}
#endif

		switch (exp[0])
		{
			case '%':
				if (exp[1] >= '0' && exp[1] <= '9')
				{
					if (exp[2] == 0)
					{
						strcpy(gtd->vars[exp[1] - '0'], str);

						return TRUE;
					}

					for (cnt = 0 ; ; cnt++)
					{
						if (action_regexp(exp + 2, &str[cnt]))
						{
							memcpy(gtd->vars[exp[1] - '0'], str, cnt);
							gtd->vars[exp[1] - '0'][cnt] = 0;

							return TRUE;
						}
						if (str[cnt] == 0)
						{
							return FALSE;
						}
					}
				}
				break;

			case '^':
				if (exp[1] == 0)
				{
					return (exp[1] == str[0]);
				}
				break;
		}

		if (*exp != *str)
		{
			return FALSE;
		}
		exp++;
		str++;
	}
	return TRUE;
}
