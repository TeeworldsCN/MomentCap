#include <engine/shared/config.h>
#include <game/server/gamecontext.h>

#include "posinghelper.h"

short CPoseCharacter::s_FakeClientIDs[MAX_CLIENTS][FAKE_MAX_CLIENTS];
short CPoseCharacter::s_LastSnapID = 0;
CGameWorld *CPoseCharacter::s_pGameWorld = NULL;
std::unordered_map<std::string, CPoseCharacter> CPoseCharacter::s_PoseMap;
std::unordered_map<std::string, int> CPoseCharacter::s_AddressCount;

void CPoseCharacter::SnapPoses(int SnappingClient)
{
	for(auto &Pose : s_PoseMap)
	{
		Pose.second.Snap(SnappingClient);
	}
}

bool CPoseCharacter::CanModify(CPlayer *pPlayer)
{
	std::string Key(Server()->ClientName(pPlayer->GetCID()));
	bool Exists = HasPose(pPlayer);

	if(!Exists)
		return true;

	auto &Pose = s_PoseMap[Key];

	if(Exists && str_comp(Pose.m_aTimeoutCode, pPlayer->m_TimeoutCode) != 0 && Pose.m_aTimeoutCode[0] != 0)
	{
		GameServer()->SendChatTarget(pPlayer->GetCID(), "抱歉，您没有权限修改这个ID的占位。请尝试重新加入服务器，如果这个提示一直出现，请联系CHN服管理员。联系QQ群：1044036098，或发送邮件给：2021@teeworlds.cn");
		return false;
	}

	return true;
}

void CPoseCharacter::Init(CGameWorld *pGameWorld)
{
	s_pGameWorld = pGameWorld;
	mem_zero(s_FakeClientIDs, sizeof(s_FakeClientIDs));
}

bool CPoseCharacter::HasPose(CPlayer *pPlayer)
{
	std::string Key(Server()->ClientName(pPlayer->GetCID()));
	if(s_PoseMap.count(Key))
		return true;
	return false;
}

bool CPoseCharacter::RemovePose(CPlayer *pPlayer)
{
	if(!CanModify(pPlayer))
		return false;

	std::string Key(Server()->ClientName(pPlayer->GetCID()));
	if(s_PoseMap.count(Key))
	{
		s_AddressCount[s_PoseMap[Key].m_aAddr]--;
		s_PoseMap.erase(Key);
	}

	return true;
}

bool CPoseCharacter::RemovePoseByName(const char *pName)
{
	std::string Key(pName);
	if(s_PoseMap.count(Key))
	{
		s_AddressCount[s_PoseMap[Key].m_aAddr]--;
		s_PoseMap.erase(Key);
		return true;
	}

	return false;
}

bool CPoseCharacter::Pose(CPlayer *pPlayer)
{
	if(!CanModify(pPlayer))
		return false;

	char aAddr[NETADDR_MAXSTRSIZE];
	Server()->GetClientAddr(pPlayer->GetCID(), aAddr, NETADDR_MAXSTRSIZE);
	auto AddressCount = s_AddressCount[aAddr];

	std::string Key(Server()->ClientName(pPlayer->GetCID()));
	bool Exists = HasPose(pPlayer);

	if(!Exists && AddressCount >= g_Config.m_SvMaxCapturePerIP)
	{
		GameServer()->SendChatTarget(pPlayer->GetCID(), "抱歉，您占位数量太多啦。请取消一个占位再试。");
		return false;
	}

	// check distance WARNING: SLOWWWWWW
	if(pPlayer->GetCharacter())
	{
		vec2 Pos = pPlayer->GetCharacter()->m_Pos;
		for(auto &Pose : s_PoseMap)
		{
			if(Pose.first.compare(Key) != 0 && distance(vec2(Pose.second.m_Core.m_X, Pose.second.m_Core.m_Y), Pos) < 100.0f)
			{
				GameServer()->SendChatTarget(pPlayer->GetCID(), "站在这里名字会被挡住哦。要不换个地方吧。");
				return false;
			}
		}
	}

	auto &Pose = s_PoseMap[Key];

	if(pPlayer->GetCharacter())
	{
		if(Exists)
			s_AddressCount[Pose.m_aAddr]--;

		Pose.WriteCharacter(pPlayer->GetCharacter());
		Pose.WritePlayer(pPlayer);
		s_AddressCount[Pose.m_aAddr]++;
		Pose.m_Init = true;
	}

	if(!Pose.m_Init)
	{
		s_PoseMap.erase(Key);
	}

	return true;
}

int CPoseCharacter::FindIDFor(int SnappingClient)
{
	for(int i = 1; i < MAX_CLIENTS; ++i)
	{
		if(s_FakeClientIDs[SnappingClient][i] < s_LastSnapID - 1)
		{
			return i;
		}
	}
	return -1;
}

bool CPoseCharacter::IsCurrent(int SnappingClient, int FakeID)
{
	if(FakeID == 0)
		return false;
	if(s_FakeClientIDs[SnappingClient][FakeID] == s_LastSnapID)
		return true;

	return false;
}

CPoseCharacter::CPoseCharacter()
{
	m_Init = false;
	mem_zero(m_ClientPoseMap, sizeof(m_ClientPoseMap));
}

int CPoseCharacter::NetworkClipped(int SnappingClient)
{
	return NetworkClipped(SnappingClient, vec2(m_Core.m_X, m_Core.m_Y));
}

int CPoseCharacter::NetworkClipped(int SnappingClient, vec2 CheckPos)
{
	if(SnappingClient == -1 || GameServer()->m_apPlayers[SnappingClient]->m_ShowAll)
		return 0;

	float dx = GameServer()->m_apPlayers[SnappingClient]->m_ViewPos.x - CheckPos.x;
	if(absolute(dx) > GameServer()->m_apPlayers[SnappingClient]->m_ShowDistance.x)
		return 1;

	float dy = GameServer()->m_apPlayers[SnappingClient]->m_ViewPos.y - CheckPos.y;
	if(absolute(dy) > GameServer()->m_apPlayers[SnappingClient]->m_ShowDistance.y)
		return 1;

	return 0;
}

void CPoseCharacter::WriteCharacter(CCharacter *pCharacter)
{
	pCharacter->Core()->Write(&m_Core);
	m_Weapon = pCharacter->Core()->m_ActiveWeapon;
	m_EmoteType = pCharacter->GetEmoteType();
}

void CPoseCharacter::WritePlayer(CPlayer *pPlayer)
{
	StrToInts(&m_ClientInfo.m_Name0, 4, Server()->ClientName(pPlayer->GetCID()));
	StrToInts(&m_ClientInfo.m_Clan0, 3, Server()->ClientClan(pPlayer->GetCID()));
	m_ClientInfo.m_Country = Server()->ClientCountry(pPlayer->GetCID());
	StrToInts(&m_ClientInfo.m_Skin0, 6, pPlayer->m_TeeInfos.m_SkinName);
	m_ClientInfo.m_UseCustomColor = pPlayer->m_TeeInfos.m_UseCustomColor;
	m_ClientInfo.m_ColorBody = pPlayer->m_TeeInfos.m_ColorBody;
	m_ClientInfo.m_ColorFeet = pPlayer->m_TeeInfos.m_ColorFeet;

	Server()->GetClientAddr(pPlayer->GetCID(), m_aAddr, NETADDR_MAXSTRSIZE);
	str_copy(m_aTimeoutCode, pPlayer->m_TimeoutCode, 64);
}

void CPoseCharacter::SavePoses()
{
	GameServer()->Storage()->CreateFolder("poses", IStorage::TYPE_SAVE);

	char aTimestamp[32];
	str_timestamp(aTimestamp, sizeof(aTimestamp));
	char aBufTime[64];
	char aBufNew[64];
	str_format(aBufTime, sizeof(aBufTime), "poses/%s.poses", aTimestamp);
	str_format(aBufNew, sizeof(aBufNew), "poses/latest.poses");

	char aBuf[255];
	str_format(aBuf, sizeof(aBuf), "backing up poses \"%s\"", aBufTime);

	GameServer()->Storage()->RenameFile(aBufNew, aBufTime, IStorage::TYPE_SAVE);
	GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "poses", aBuf);

	size_t Size = s_PoseMap.size();
	if(Size <= 0)
		return;

	str_format(aBuf, sizeof(aBuf), "saving poses \"%s\"", aBufNew);
	GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "poses", aBuf);
	IOHANDLE File = GameServer()->Storage()->OpenFile(aBufNew, IOFLAG_WRITE, IStorage::TYPE_SAVE);
	if(!File)
	{
		str_format(aBuf, sizeof(aBuf), "poses saving failed");
		GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "poses", aBuf);
		return;
	}

	io_write(File, &Size, sizeof(size_t));

	for(const auto Pose : s_PoseMap)
	{
		io_write(File, &Pose.second.m_ClientInfo, sizeof(m_ClientInfo));
		io_write(File, &Pose.second.m_Core, sizeof(m_Core));
		io_write(File, &Pose.second.m_EmoteType, sizeof(m_EmoteType));
		io_write(File, &Pose.second.m_Weapon, sizeof(m_Weapon));
		io_write(File, Pose.second.m_aTimeoutCode, sizeof(m_aTimeoutCode));
		io_write(File, Pose.second.m_aAddr, sizeof(m_aAddr));
	}

	io_close(File);
}

const CPoseCharacter *CPoseCharacter::ClosestPose(vec2 Pos, float Radius)
{
	const CPoseCharacter *pClosest = NULL;
	float MinDist = INFINITY;
	for(const auto &Pose : s_PoseMap)
	{
		float Dist = distance(vec2(Pose.second.m_Core.m_X, Pose.second.m_Core.m_Y), Pos);
		if(Dist < Radius && Dist < MinDist)
		{
			MinDist = Dist;
			pClosest = &(Pose.second);
		}
	}

	return pClosest;
}

void CPoseCharacter::LoadPoses()
{
	s_PoseMap.clear();
	s_AddressCount.clear();

	char aFile[64];
	str_format(aFile, sizeof(aFile), "poses/latest.poses");

	char aBuf[255];
	str_format(aBuf, sizeof(aBuf), "reading poses \"%s\"", aFile);
	GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "poses", aBuf);

	IOHANDLE File = GameServer()->Storage()->OpenFile(aFile, IOFLAG_READ, IStorage::TYPE_SAVE);
	if(!File)
	{
		str_format(aBuf, sizeof(aBuf), "poses reading failed");
		GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "poses", aBuf);
		return;
	}

	int NumPoses = 0;
	io_read(File, &NumPoses, sizeof(size_t));

	for(int i = 0; i < NumPoses; ++i)
	{
		CNetObj_ClientInfo ClientInfo;
		io_read(File, &ClientInfo, sizeof(m_ClientInfo));

		char aName[MAX_NAME_LENGTH];
		IntsToStr(&ClientInfo.m_Name0, 4, aName);
		std::string Key(aName);

		auto &Pose = s_PoseMap[aName];

		Pose.m_ClientInfo = ClientInfo;
		io_read(File, &Pose.m_Core, sizeof(m_Core));
		io_read(File, &Pose.m_EmoteType, sizeof(m_EmoteType));
		io_read(File, &Pose.m_Weapon, sizeof(m_Weapon));
		io_read(File, &Pose.m_aTimeoutCode, sizeof(m_aTimeoutCode));
		io_read(File, &Pose.m_aAddr, sizeof(m_aAddr));
		s_AddressCount[Pose.m_aAddr]++;
		Pose.m_Init = true;
	}

	io_close(File);
}

void CPoseCharacter::SnapCharacter(int SnappingClient, int ID)
{
	int Health = 10, Armor = 0, AmmoCount = 0;

	if(!Server()->IsSixup(SnappingClient))
	{
		CNetObj_Character *pCharacter = static_cast<CNetObj_Character *>(Server()->SnapNewItem(NETOBJTYPE_CHARACTER, ID, sizeof(CNetObj_Character)));
		if(!pCharacter)
			return;

		pCharacter->m_X = m_Core.m_X;
		pCharacter->m_Y = m_Core.m_Y;
		pCharacter->m_VelX = 0;
		pCharacter->m_VelY = 0;
		pCharacter->m_Angle = m_Core.m_Angle;
		pCharacter->m_Direction = 0;
		pCharacter->m_Jumped = 0;
		pCharacter->m_HookState = m_Core.m_HookState;
		pCharacter->m_HookTick = 0;
		pCharacter->m_HookX = m_Core.m_HookX;
		pCharacter->m_HookY = m_Core.m_HookY;
		pCharacter->m_HookDx = m_Core.m_HookDx;
		pCharacter->m_HookDy = m_Core.m_HookDy;

		pCharacter->m_Tick = Server()->Tick();
		pCharacter->m_Emote = m_EmoteType;
		pCharacter->m_HookedPlayer = -1;
		pCharacter->m_AttackTick = 0;
		pCharacter->m_Weapon = m_Weapon;
		pCharacter->m_AmmoCount = AmmoCount;
		pCharacter->m_Health = Health;
		pCharacter->m_Armor = Armor;
		pCharacter->m_PlayerFlags = 0;
	}
	else
	{
		protocol7::CNetObj_Character *pCharacter = static_cast<protocol7::CNetObj_Character *>(Server()->SnapNewItem(NETOBJTYPE_CHARACTER, ID, sizeof(protocol7::CNetObj_Character)));
		if(!pCharacter)
			return;

		pCharacter->m_X = m_Core.m_X;
		pCharacter->m_Y = m_Core.m_Y;
		pCharacter->m_VelX = 0;
		pCharacter->m_VelY = 0;
		pCharacter->m_Angle = m_Core.m_Angle;
		pCharacter->m_Direction = 0;
		pCharacter->m_Jumped = 0;
		pCharacter->m_HookState = m_Core.m_HookState;
		pCharacter->m_HookTick = m_Core.m_HookTick;
		pCharacter->m_HookX = m_Core.m_HookX;
		pCharacter->m_HookY = m_Core.m_HookY;
		pCharacter->m_HookDx = m_Core.m_HookDx;
		pCharacter->m_HookDy = m_Core.m_HookDy;

		pCharacter->m_Tick = Server()->Tick();
		pCharacter->m_Emote = m_EmoteType;
		pCharacter->m_AttackTick = 0;
		pCharacter->m_Weapon = m_Weapon;
		pCharacter->m_AmmoCount = AmmoCount;
		pCharacter->m_Health = Health;
		pCharacter->m_Armor = Armor;
		pCharacter->m_TriggeredEvents = 0;
	}
}

void CPoseCharacter::Snap(int SnappingClient)
{
	int _id = 0;
	if(SnappingClient > -1 && !Server()->Translate(_id, SnappingClient))
		return;

	if(NetworkClipped(SnappingClient))
		return;

	IServer::CClientInfo Info;
	Server()->GetClientInfo(SnappingClient, &Info);
	bool IsOld = Info.m_DDNetVersion < VERSION_DDNET_OLD;

	int ID = m_ClientPoseMap[SnappingClient];
	if(!IsCurrent(SnappingClient, ID))
		ID = FindIDFor(SnappingClient);

	// leave one for spec
	if(ID <= 0 || (IsOld && ID >= VANILLA_MAX_CLIENTS) || ID >= FAKE_MAX_CLIENTS)
		return;

	SnapCharacter(SnappingClient, ID);

	CNetObj_DDNetCharacter *pDDNetCharacter = static_cast<CNetObj_DDNetCharacter *>(Server()->SnapNewItem(NETOBJTYPE_DDNETCHARACTER, ID, sizeof(CNetObj_DDNetCharacter)));
	if(!pDDNetCharacter)
		return;

	pDDNetCharacter->m_Flags = 0;
	pDDNetCharacter->m_Flags |= CHARACTERFLAG_SOLO;
	pDDNetCharacter->m_Flags |= CHARACTERFLAG_NO_COLLISION;
	pDDNetCharacter->m_Flags |= CHARACTERFLAG_NO_HOOK;
	pDDNetCharacter->m_Flags |= CHARACTERFLAG_NO_HAMMER_HIT;
	pDDNetCharacter->m_Flags |= CHARACTERFLAG_NO_GRENADE_HIT;
	pDDNetCharacter->m_Flags |= CHARACTERFLAG_NO_LASER_HIT;
	pDDNetCharacter->m_Flags |= CHARACTERFLAG_NO_SHOTGUN_HIT;
	pDDNetCharacter->m_Flags |= CHARACTERFLAG_WEAPON_HAMMER;
	pDDNetCharacter->m_Flags |= CHARACTERFLAG_WEAPON_GUN;
	pDDNetCharacter->m_Flags |= CHARACTERFLAG_WEAPON_SHOTGUN;
	pDDNetCharacter->m_Flags |= CHARACTERFLAG_WEAPON_GRENADE;
	pDDNetCharacter->m_Flags |= CHARACTERFLAG_WEAPON_LASER;

	pDDNetCharacter->m_FreezeEnd = 0;
	pDDNetCharacter->m_Jumps = 1;
	pDDNetCharacter->m_TeleCheckpoint = 0;
	pDDNetCharacter->m_StrongWeakID = 0;

	SnapPlayer(SnappingClient, ID);

	s_FakeClientIDs[SnappingClient][ID] = s_LastSnapID + 1;
	m_ClientPoseMap[SnappingClient] = ID;
}

void CPoseCharacter::SnapPlayer(int SnappingClient, int ID)
{
	CNetObj_ClientInfo *pClientInfo = static_cast<CNetObj_ClientInfo *>(Server()->SnapNewItem(NETOBJTYPE_CLIENTINFO, ID, sizeof(CNetObj_ClientInfo)));
	if(!pClientInfo)
		return;

	*pClientInfo = m_ClientInfo;

	int Latency = 999;
	int Score = -60;

	if(SnappingClient < 0 || !Server()->IsSixup(SnappingClient))
	{
		CNetObj_PlayerInfo *pPlayerInfo = static_cast<CNetObj_PlayerInfo *>(Server()->SnapNewItem(NETOBJTYPE_PLAYERINFO, ID, sizeof(CNetObj_PlayerInfo)));
		if(!pPlayerInfo)
			return;

		pPlayerInfo->m_Latency = Latency;
		pPlayerInfo->m_Score = Score;
		pPlayerInfo->m_Local = 0;
		pPlayerInfo->m_ClientID = ID;
		pPlayerInfo->m_Team = 0;
	}
	else
	{
		protocol7::CNetObj_PlayerInfo *pPlayerInfo = static_cast<protocol7::CNetObj_PlayerInfo *>(Server()->SnapNewItem(NETOBJTYPE_PLAYERINFO, ID, sizeof(protocol7::CNetObj_PlayerInfo)));
		if(!pPlayerInfo)
			return;

		pPlayerInfo->m_PlayerFlags = 0;
		pPlayerInfo->m_Score = Score * 1000;
		pPlayerInfo->m_Latency = Latency;
	}

	// if(m_ClientID == SnappingClient && (m_Team == TEAM_SPECTATORS || m_Paused))
	// {
	// 	if(SnappingClient < 0 || !Server()->IsSixup(SnappingClient))
	// 	{
	// 		CNetObj_SpectatorInfo *pSpectatorInfo = static_cast<CNetObj_SpectatorInfo *>(Server()->SnapNewItem(NETOBJTYPE_SPECTATORINFO, 0, sizeof(CNetObj_SpectatorInfo)));
	// 		if(!pSpectatorInfo)
	// 			return;

	// 		pSpectatorInfo->m_SpectatorID = m_SpectatorID;
	// 		pSpectatorInfo->m_X = m_ViewPos.x;
	// 		pSpectatorInfo->m_Y = m_ViewPos.y;
	// 	}
	// 	else
	// 	{
	// 		protocol7::CNetObj_SpectatorInfo *pSpectatorInfo = static_cast<protocol7::CNetObj_SpectatorInfo *>(Server()->SnapNewItem(NETOBJTYPE_SPECTATORINFO, 0, sizeof(protocol7::CNetObj_SpectatorInfo)));
	// 		if(!pSpectatorInfo)
	// 			return;

	// 		pSpectatorInfo->m_SpecMode = m_SpectatorID == SPEC_FREEVIEW ? protocol7::SPEC_FREEVIEW : protocol7::SPEC_PLAYER;
	// 		pSpectatorInfo->m_SpectatorID = m_SpectatorID;
	// 		pSpectatorInfo->m_X = m_ViewPos.x;
	// 		pSpectatorInfo->m_Y = m_ViewPos.y;
	// 	}
	// }

	CNetObj_DDNetPlayer *pDDNetPlayer = static_cast<CNetObj_DDNetPlayer *>(Server()->SnapNewItem(NETOBJTYPE_DDNETPLAYER, ID, sizeof(CNetObj_DDNetPlayer)));
	if(!pDDNetPlayer)
		return;

	pDDNetPlayer->m_AuthLevel = 0;
	pDDNetPlayer->m_Flags = 0;
}
