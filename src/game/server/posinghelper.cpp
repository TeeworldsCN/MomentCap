#include <game/server/gamecontext.h>

#include "posinghelper.h"

int CPoseCharacter::s_FakeClientIDs[MAX_CLIENTS];

CPoseCharacter::CPoseCharacter(CGameWorld *pGameWorld) :
	CEntity(pGameWorld, CGameWorld::ENTTYPE_POSE)
{
	GameWorld()->InsertEntity(this);
}

int CPoseCharacter::NetworkClipped(int SnappingClient)
{
	return NetworkClipped(SnappingClient, m_Pos);
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
    m_Pos = pCharacter->m_Pos;
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
        pCharacter->m_Jumped = m_Core.m_Jumped;
        pCharacter->m_HookState = m_Core.m_HookState;
        pCharacter->m_HookTick = m_Core.m_HookTick;
        pCharacter->m_HookX = m_Core.m_HookX;
        pCharacter->m_HookY = m_Core.m_HookY;
        pCharacter->m_HookDx = m_Core.m_HookDx;
        pCharacter->m_HookDy = m_Core.m_HookDy;

		pCharacter->m_Tick = 0;
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
	int _id;
	if(SnappingClient > -1 && !Server()->Translate(_id, SnappingClient))
		return;

	if(NetworkClipped(SnappingClient))
		return;

    IServer::CClientInfo Info;
	Server()->GetClientInfo(SnappingClient, &Info);
	bool IsOld = Info.m_DDNetVersion < VERSION_DDNET_OLD;

    int ID = ++s_FakeClientIDs[SnappingClient];
    if (IsOld && ID >= VANILLA_MAX_CLIENTS || ID >= MAX_CLIENTS)
        return;

	SnapCharacter(SnappingClient, ID);

	CNetObj_DDNetCharacter *pDDNetCharacter = static_cast<CNetObj_DDNetCharacter *>(Server()->SnapNewItem(NETOBJTYPE_DDNETCHARACTER, ID, sizeof(CNetObj_DDNetCharacter)));
	if(!pDDNetCharacter)
		return;

	pDDNetCharacter->m_Flags = 0;
	pDDNetCharacter->m_Flags |= CHARACTERFLAG_SOLO;
	pDDNetCharacter->m_Flags |= CHARACTERFLAG_NO_COLLISION;
	pDDNetCharacter->m_Flags |= CHARACTERFLAG_WEAPON_HAMMER;
	pDDNetCharacter->m_Flags |= CHARACTERFLAG_WEAPON_GUN;
	pDDNetCharacter->m_Flags |= CHARACTERFLAG_WEAPON_SHOTGUN;
	pDDNetCharacter->m_Flags |= CHARACTERFLAG_WEAPON_GRENADE;
	pDDNetCharacter->m_Flags |= CHARACTERFLAG_WEAPON_LASER;

	pDDNetCharacter->m_FreezeEnd = 0;
	pDDNetCharacter->m_Jumps = m_Core.m_Jumped ? 1 : 0;
	pDDNetCharacter->m_TeleCheckpoint = 0;
	pDDNetCharacter->m_StrongWeakID = 0;

    SnapPlayer(SnappingClient, ID);
}

void CPoseCharacter::SnapPlayer(int SnappingClient, int ID)
{
	CNetObj_ClientInfo *pClientInfo = static_cast<CNetObj_ClientInfo *>(Server()->SnapNewItem(NETOBJTYPE_CLIENTINFO, ID, sizeof(CNetObj_ClientInfo)));
	if(!pClientInfo)
		return;

	*pClientInfo = m_ClientInfo;

	int Latency = 999;
	int Score = 60;

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

void CPoseCharacter::ResetClientIDs()
{
    mem_zero(s_FakeClientIDs, sizeof(s_FakeClientIDs));
}