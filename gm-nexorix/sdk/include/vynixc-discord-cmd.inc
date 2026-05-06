#if defined _included_vynixc_discord_cmd
	#endinput
#endif
#define _included_vynixc_discord_cmd
#if !defined VCM_PREFIX
	#define VCM_PREFIX '!'
#endif
#define VCM:%0(%1,%2,%3) \
	forward VCM_%0(DCC_User:%1, DCC_Channel:%2, %3); \
	public VCM_%0(DCC_User:%1, DCC_Channel:%2, %3)
#if !defined isnull
	#define isnull(%0) (!(%0[0]) || (%0[0] == '\1' && !(%0[1])))
#endif
#tryinclude <discord-connector>
const VCM_TOTAL_CMD_SIZE = 256;
#if defined OnVynixcCommandAttempt
	forward OnVynixcCommandAttempt(DCC_User:user, DCC_Channel:channel, cmdtext[]);
#endif
#if defined OnVynixcCommandPerformed
	forward OnVynixcCommandPerformed(DCC_User:user, DCC_Channel:channel, cmdtext[], success);
#endif
static DCC_Message:g_VCMMessageID = DCC_Message:0;
stock DCC_Message:VCM_GetCommandMessageId() {
	return g_VCMMessageID;
}

#if !defined VCM_ALIAS_MAX
	#define VCM_ALIAS_MAX 32
#endif
static VCM_ALIAS_COUNT = 0;
static VCM_ALIAS_FROM[VCM_ALIAS_MAX][32];
static VCM_ALIAS_TO[VCM_ALIAS_MAX][32];
stock VCM_ToLower(str[]) {
	new i;
	while(str[i]) {
		if(str[i] >= 'A' && str[i] <= 'Z') {
			str[i] |= 0x20;
		}
		i++;
	}
	return 1;
}

stock VCM_RegisterAlias(const alias[], const target[]) {
	if(VCM_ALIAS_COUNT >= VCM_ALIAS_MAX) return 0;

	new idx = VCM_ALIAS_COUNT;

	VCM_ALIAS_FROM[idx][0] = '\0';
	VCM_ALIAS_TO[idx][0] = '\0';

	strcat(VCM_ALIAS_FROM[idx], alias, sizeof(VCM_ALIAS_FROM[]));
	strcat(VCM_ALIAS_TO[idx], target, sizeof(VCM_ALIAS_TO[]));

	#if !defined VCM_STRICT_CASE
		VCM_ToLower(VCM_ALIAS_FROM[idx]);
		VCM_ToLower(VCM_ALIAS_TO[idx]);
	#endif

	VCM_ALIAS_COUNT++;
	return 1;
}

stock VCM_ClearAliases() {
	VCM_ALIAS_COUNT = 0;
	return 1;
}

stock VCM_ResolveAlias(const name[], dest[], size = 32) {
	new i;
	for(i = 0; i < VCM_ALIAS_COUNT; i++) {
		if(!strcmp(name, VCM_ALIAS_FROM[i])) {
			dest[0] = '\0';
			strcat(dest, VCM_ALIAS_TO[i], size);
			return 1;
		}
	}
	dest[0] = '\0';
	strcat(dest, name, size);
	return 0;
}

stock ParamCount(const params[]) {
	if(isnull(params)) return 0;

	new i, count = 0, bool:inTok = false, bool:inQ = false, q = '\0', c;

	while((c = params[i])) {
		if(!inQ) {
			if(c == '"' || c == '\'') {
				inQ = true;
				q = c;
				if(!inTok) inTok = true;
			} else if(c <= ' ') {
				if(inTok) {
					count++;
					inTok = false;
				}
			} else {
				if(!inTok) inTok = true;
			}
		} else {
			if(c == q) {
				inQ = false;
			}
		}
		i++;
	}

	if(inTok) count++;
	return count;
}

stock GetParam(const params[], index, dest[], len) {
	if(isnull(params)) return 0;

	new i, cur = 0, pos = 0, bool:inQ = false, q = '\0', c;

	while(params[i]) {
		while(params[i] <= ' ' && params[i]) i++;
		if(!params[i]) break;

		inQ = false;

		if(params[i] == '"' || params[i] == '\'') {
			inQ = true;
			q = params[i];
			i++;
		}

		if(cur == index) {
			while((c = params[i])) {
				if(inQ) {
					if(c == q) {
						i++;
						break;
					}
					if(pos < len - 1) dest[pos++] = c;
					i++;
				} else {
					if(c <= ' ') break;
					if(pos < len - 1) dest[pos++] = c;
					i++;
				}
			}
			dest[pos] = '\0';
			return 1;
		} else {
			while((c = params[i])) {
				if(inQ) {
					if(c == q) {
						i++;
						break;
					}
				} else {
					if(c <= ' ') break;
				}
				i++;
			}
			cur++;
		}
	}
	return 0;
}

stock GetInt(params[], index) {
	new b[64];
	if(!GetParam(params, index, b, sizeof(b))) return 0;
	return strval(b);
}

stock Float:GetFloat(params[], index) {
	new b[64];
	if(!GetParam(params, index, b, sizeof(b))) return 0.0;
	return floatstr(b);
}

stock bool:GetBool(params[], index) {
	new b[16];
	if(!GetParam(params, index, b, sizeof(b))) return false;
	#if !defined VCM_STRICT_CASE
		VCM_ToLower(b);
	#endif
	if(!strcmp(b, "true") || !strcmp(b, "yes") || !strcmp(b, "on") || !strcmp(b, "1")) return true;
	return false;
}

stock DCC_User:VCM_GetParamUser(const params[], index) {
	new buf[64];
	if(!GetParam(params, index, buf, sizeof(buf))) return DCC_User:0;
	new start = 0;
	if(buf[0] == '<' && buf[1] == '@') {
		start = 2;
		if(buf[2] == '!') start = 3;
		new len = strlen(buf);
		if(buf[len-1] == '>') buf[len-1] = '\0';
	}
	new DCC_User:user = DCC_FindUserById(buf[start]);
	return user;
}

#if defined DCC_FindRoleById && defined DCC_HasGuildMemberRole
stock bool:VCM_HasRole(DCC_Guild:guild, DCC_User:user, const roleId[]) {
	new DCC_Role:role;
	role = DCC_FindRoleById(roleId);
	if(!role) return false;
	new bool:has;
	DCC_HasGuildMemberRole(guild, user, role, has);
	return has;
}
#endif

stock VCM_GetTimeFromString(const str[]) {
	new len = strlen(str);
	if(len < 2) return strval(str);
	
	new unit = str[len-1];
	new val = strval(str);
	
	if(unit == 's' || unit == 'S') return val;
	if(unit == 'm' || unit == 'M') return val * 60;
	if(unit == 'h' || unit == 'H') return val * 3600;
	if(unit == 'd' || unit == 'D') return val * 86400;
	if(unit == 'w' || unit == 'W') return val * 604800;
	
	return val;
}

#if defined GetPlayerName
stock VCM_GetPlayerByDiscord(DCC_User:user) {
	new id[21], name[25];
	DCC_GetUserId(user, id, sizeof(id));
	// Necessita de sistema próprio de vinculação, exemplo simples:
	// Loop por todos players e verificar variável de conta
	// Retornando -1 por padrão pois depende do GM
	return -1; 
}
#endif

#if defined DCC_SendChannelMessage
stock Send(DCC_Channel:channel, const message[]) {
	return DCC_SendChannelMessage(channel, message);
}

stock Usage(DCC_Channel:channel, const usage[]) {
	return DCC_SendChannelMessage(channel, usage);
}

#endif
#if defined DCC_CreateEmbed && defined DCC_SendChannelEmbedMessage
stock SendSimpleEmbed(DCC_Channel:channel, const title[], const description[], color = 0x3498DB) {
	new DCC_Embed:embed = DCC_CreateEmbed(title, description, "", "", color);
	DCC_SendChannelEmbedMessage(channel, embed);
	return 1;
}

stock DCC_Embed:VCM_CreateEmbed(const title[], const description[], color = 0x3498DB) {
	return DCC_CreateEmbed(title, description, "", "", color);
}

stock VCM_AddEmbedField(DCC_Embed:embed, const name[], const value[], bool:inline = false) {
	return DCC_AddEmbedField(embed, name, value, inline);
}

stock VCM_SetEmbedFooter(DCC_Embed:embed, const text[]) {
	return DCC_SetEmbedFooter(embed, text);
}

stock VCM_SetEmbedImage(DCC_Embed:embed, const url[]) {
	return DCC_SetEmbedImage(embed, url);
}

stock VCM_SetEmbedThumbnail(DCC_Embed:embed, const url[]) {
	return DCC_SetEmbedThumbnail(embed, url);
}

stock VCM_SendEmbed(DCC_Channel:channel, DCC_Embed:embed) {
	return DCC_SendChannelEmbedMessage(channel, embed);
}
#endif

stock DCC_Channel:VCM_GetChannelById(const channelId[]) {
	new DCC_Channel:channel = DCC_FindChannelById(channelId);
	return channel;
}

stock VCM_SendToChannelID(const channelId[], const message[]) {
	new DCC_Channel:channel = VCM_GetChannelById(channelId);
	if(!channel) return 0;
	return DCC_SendChannelMessage(channel, message);
}

stock VCM_SendLogToChannelID(const channelId[], const title[], const text[], color = 0x3498DB) {
	new DCC_Channel:channel = VCM_GetChannelById(channelId);
	if(!channel) return 0;
	return SendSimpleEmbed(channel, title, text, color);
}

#if defined GetPlayerName
stock VCM_SendGameChatToDiscord(const channelId[], playerid, const text[]) {
	new name[24], str[256];
	GetPlayerName(playerid, name, sizeof(name));
	format(str, sizeof(str), "**%s**: %s", name, text);
	return VCM_SendToChannelID(channelId, str);
}

stock VCM_SendCommandLogToDiscord(const channelId[], playerid, const cmdtext[]) {
	new name[24], str[256];
	GetPlayerName(playerid, name, sizeof(name));
	format(str, sizeof(str), "Player: %s\nCMD: %s", name, cmdtext);
	return VCM_SendLogToChannelID(channelId, "Command Log", str, 0xF1C40F);
}
#endif

public DCC_OnMessageCreate(DCC_Message:message) {
	new DCC_User:user, DCC_Channel:channel, cmdtext[VCM_TOTAL_CMD_SIZE], bool:hasParams = false, bool:isBot;
	DCC_GetMessageContent(message, cmdtext, sizeof(cmdtext));
	new bool:prefixMatched = false, start = 0;
	#if defined VCM_PREFIX_STR
		new plen = strlen(VCM_PREFIX_STR);
		if(!strncmp(cmdtext, VCM_PREFIX_STR, plen)) { prefixMatched = true; start = plen; }
	#endif
	#if !defined VCM_PREFIX_STR
		if(cmdtext[0] == VCM_PREFIX) { prefixMatched = true; start = 1; }
	#endif
	if(prefixMatched) {
		DCC_GetMessageAuthor(message, user);
		DCC_GetMessageChannel(message, channel);
		DCC_IsUserBot(user, isBot);
		#if !defined VCM_ALLOW_BOTS
			if(isBot) return 0;
		#endif
		g_VCMMessageID = message;
		#if defined OnVynixcCommandAttempt
			if(!OnVynixcCommandAttempt(user, channel, cmdtext[start])) return 0;
		#endif
		static command[32] = "VCM_", success = 0;
		new pos = start, cmdname[32], ci = 0;
		while(ci < 31 && cmdtext[pos] > ' ') {
			#if defined VCM_STRICT_CASE
				cmdname[ci++] = cmdtext[pos];
			#else
				cmdname[ci++] = ((cmdtext[pos] >= 'A' && cmdtext[pos] <= 'Z') ? (cmdtext[pos] | 0x20) : cmdtext[pos]);
			#endif
			pos++;
		}
		cmdname[ci] = '\0';
		while(cmdtext[pos] == ' ') ++pos;
		if(cmdtext[pos]) hasParams = true;
		new resolved[32];
		VCM_ResolveAlias(cmdname, resolved, sizeof(resolved));
		new idx = 4, j = 0;
		while(idx < 31 && resolved[j]) {
			command[idx++] = resolved[j++];
		}
		command[idx] = '\0';
		if(hasParams) {
			success = CallLocalFunction(command, "iis", _:user, _:channel, cmdtext[pos]);
		} else {
			success = CallLocalFunction(command, "iis", _:user, _:channel, "\1");
		}
		#if defined OnVynixcCommandPerformed
			return OnVynixcCommandPerformed(user, channel, cmdtext[start], success);
		#else
			return 1;
		#endif
		#pragma unused success
	}
	#if defined VCM_OnMessageCreate
		return VCM_OnMessageCreate(message);
	#else
		return 1;
	#endif
}

#if defined _ALS_OnMessageCreate
	#undef DCC_OnMessageCreate
#else
	#define _ALS_OnMessageCreate
#endif

#define DCC_OnMessageCreate VCM_OnMessageCreate
#if defined VCM_OnMessageCreate
	forward VCM_OnMessageCreate(DCC_Message:message);
#endif
