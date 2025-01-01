#include <engine/shared/config.h>
#include <game/server/gamecontext.h>

#include "posinghelper.h"

bool CPoseCharacter::s_SnapCached[MAX_CLIENTS];
int CPoseCharacter::s_FakeEntityIDs[FAKE_MAX_CLIENTS];
SPoseSnapCache CPoseCharacter::s_SnapCache[MAX_CLIENTS][FAKE_MAX_CLIENTS];

uint32_t CPoseCharacter::s_FakeClientIDs[MAX_CLIENTS][FAKE_MAX_CLIENTS];
uint32_t CPoseCharacter::s_LastSnapID = 0;

CGameWorld *CPoseCharacter::s_pGameWorld = NULL;
std::unordered_map<std::string, CPoseCharacter> CPoseCharacter::s_PoseMap;
std::unordered_map<int, std::set<std::string>> CPoseCharacter::s_SpatialMap;
std::unordered_set<std::string> CPoseCharacter::s_PoseSnapCache;
std::unordered_map<std::string, int> CPoseCharacter::s_AddressCount;

int CPoseCharacter::CountPosesOfSpace(vec2 Pos, int SpatialKey, bool &Allowed)
{
	if(!s_SpatialMap.count(SpatialKey))
		return 0;
	auto map = s_SpatialMap[SpatialKey];
	int result = 0;
	for(auto key : map)
	{
		if(s_PoseSnapCache.count(key))
			continue;
		s_PoseSnapCache.insert(key);
		int X = s_PoseMap[key].m_Core.m_X;
		int Y = s_PoseMap[key].m_Core.m_Y;
		float dx = Pos.x - X;
		float dy = Pos.y - Y;
		if(absolute(dx) <= g_Config.m_SvPosesAreaCheckX && absolute(dy) <= g_Config.m_SvPosesAreaCheckY)
			result++;

		if(absolute(dx) <= g_Config.m_SvPosesMinDistX && absolute(dy) <= g_Config.m_SvPosesMinDistY)
			Allowed = false;
	}
	return result;
}

int CPoseCharacter::CountPosesAround(vec2 Pos, bool &Allowed)
{
	// Spatial hash snap
	int ViewX = Pos.x;
	int ViewY = Pos.y;

	s_PoseSnapCache.clear();

	int Center = HashCoordinate(ViewX, ViewY);
	int TopLeft = HashCoordinate(ViewX, ViewY, -1, -1);
	int TopRight = HashCoordinate(ViewX, ViewY, 1, -1);
	int BottomLeft = HashCoordinate(ViewX, ViewY, -1, 1);
	int BottomRight = HashCoordinate(ViewX, ViewY, 1, 1);
	int Left = HashCoordinate(ViewX, ViewY, -1, 0);
	int Right = HashCoordinate(ViewX, ViewY, 1, 0);
	int Top = HashCoordinate(ViewX, ViewY, 0, -1);
	int Bottom = HashCoordinate(ViewX, ViewY, 0, 1);

	int result = 0;

	result += CountPosesOfSpace(Pos, Center, Allowed);
	result += CountPosesOfSpace(Pos, TopLeft, Allowed);
	result += CountPosesOfSpace(Pos, TopRight, Allowed);
	result += CountPosesOfSpace(Pos, BottomLeft, Allowed);
	result += CountPosesOfSpace(Pos, BottomRight, Allowed);
	result += CountPosesOfSpace(Pos, Left, Allowed);
	result += CountPosesOfSpace(Pos, Right, Allowed);
	result += CountPosesOfSpace(Pos, Top, Allowed);
	result += CountPosesOfSpace(Pos, Bottom, Allowed);

	return result;
}

void CPoseCharacter::SnapPosesOfSpace(int SnappingClient, int SpatialKey)
{
	if(!s_SpatialMap.count(SpatialKey))
		return;
	auto map = s_SpatialMap[SpatialKey];
	for(auto key : map)
	{
		if(s_PoseSnapCache.count(key))
			continue;
		s_PoseSnapCache.insert(key);
		s_PoseMap[key].Snap(SnappingClient);
	}
}

int CPoseCharacter::SnapPoses(int SnappingClient, bool AsSpec, bool NewSnap)
{
	int Count = 0;
	if(!Server()->IsSixup(SnappingClient))
	{
		bool NeedSnap = NewSnap || !s_SnapCached[SnappingClient];
		if(NeedSnap)
		{
			for(auto &Cache : s_SnapCache[SnappingClient])
				Cache.m_Exists = false;

			/* Replaced this with a more efficient version
			for(auto &Pose : s_PoseMap)
			{
				Pose.second.Snap(SnappingClient);
			}*/

			// Spatial hash snap
			int ViewX = GameServer()->m_apPlayers[SnappingClient]->m_ViewPos.x;
			int ViewY = GameServer()->m_apPlayers[SnappingClient]->m_ViewPos.y;
			s_PoseSnapCache.clear();

			int Center = HashCoordinate(ViewX, ViewY);
			int TopLeft = HashCoordinate(ViewX, ViewY, -1, -1);
			int TopRight = HashCoordinate(ViewX, ViewY, 1, -1);
			int BottomLeft = HashCoordinate(ViewX, ViewY, -1, 1);
			int BottomRight = HashCoordinate(ViewX, ViewY, 1, 1);
			int Left = HashCoordinate(ViewX, ViewY, -1, 0);
			int Right = HashCoordinate(ViewX, ViewY, 1, 0);
			int Top = HashCoordinate(ViewX, ViewY, 0, -1);
			int Bottom = HashCoordinate(ViewX, ViewY, 0, 1);

			SnapPosesOfSpace(SnappingClient, Center);
			SnapPosesOfSpace(SnappingClient, TopLeft);
			SnapPosesOfSpace(SnappingClient, TopRight);
			SnapPosesOfSpace(SnappingClient, BottomLeft);
			SnapPosesOfSpace(SnappingClient, BottomRight);
			SnapPosesOfSpace(SnappingClient, Left);
			SnapPosesOfSpace(SnappingClient, Right);
			SnapPosesOfSpace(SnappingClient, Top);
			SnapPosesOfSpace(SnappingClient, Bottom);

			for(auto &Cache : s_SnapCache[SnappingClient])
				Cache.m_pID = Cache.m_pExpectedID;
			s_SnapCached[SnappingClient] = true;
		}

		IServer::CClientInfo Info;
		Server()->GetClientInfo(SnappingClient, &Info);
		bool IsOld = Info.m_DDNetVersion < VERSION_DDNET_OLD;
		int MaxClientSnaps = IsOld ? VANILLA_MAX_CLIENTS : FAKE_MAX_CLIENTS;

		for(int i = 1; i < FAKE_MAX_CLIENTS; ++i)
		{
			auto *pCache = &s_SnapCache[SnappingClient][i];
			if(!pCache->m_Exists)
				continue;

			if(AsSpec || i >= MaxClientSnaps)
			{
				CNetObj_Laser *pLaser = static_cast<CNetObj_Laser *>(Server()->SnapNewItem(NETOBJTYPE_LASER, s_FakeEntityIDs[i], sizeof(CNetObj_Laser)));
				if(!pLaser)
					continue;
				*pLaser = pCache->m_Laser;
				pLaser->m_StartTick = Server()->Tick() - 4;
			}
			else
			{
				vec2 Pos = vec2(pCache->m_Char.m_X, pCache->m_Char.m_Y);
				vec2 HookPos = vec2(pCache->m_Char.m_HookX, pCache->m_Char.m_HookY);

				if(NetworkClipped(SnappingClient, Pos, HookPos))
					continue;

				if(!pCache->m_NeedSkip)
				{
					Count++;
					CNetObj_Character *pCharacter = static_cast<CNetObj_Character *>(Server()->SnapNewItem(NETOBJTYPE_CHARACTER, i, sizeof(CNetObj_Character)));
					if(!pCharacter)
						continue;
					*pCharacter = pCache->m_Char;
					pCharacter->m_Tick = Server()->Tick() - 4;

					CNetObj_DDNetCharacter *pDDNetChar = static_cast<CNetObj_DDNetCharacter *>(Server()->SnapNewItem(NETOBJTYPE_DDNETCHARACTER, i, sizeof(CNetObj_DDNetCharacter)));
					if(!pDDNetChar)
						continue;
					*pDDNetChar = pCache->m_DDNetChar;

					CNetObj_ClientInfo *pClientInfo = static_cast<CNetObj_ClientInfo *>(Server()->SnapNewItem(NETOBJTYPE_CLIENTINFO, i, sizeof(CNetObj_ClientInfo)));
					if(!pClientInfo)
						continue;

					*pClientInfo = pCache->m_ClientInfo;

					CNetObj_PlayerInfo *pPlayerInfo = static_cast<CNetObj_PlayerInfo *>(Server()->SnapNewItem(NETOBJTYPE_PLAYERINFO, i, sizeof(CNetObj_PlayerInfo)));
					if(!pPlayerInfo)
						continue;

					*pPlayerInfo = pCache->m_PlayerInfo;

					CNetObj_DDNetPlayer *pDDNetPlayer = static_cast<CNetObj_DDNetPlayer *>(Server()->SnapNewItem(NETOBJTYPE_DDNETPLAYER, i, sizeof(CNetObj_DDNetPlayer)));
					if(!pDDNetPlayer)
						continue;
					*pDDNetPlayer = pCache->m_DDNetPlayer;
				}
				else
				{
					pCache->m_NeedSkip = false;
				}

				s_FakeClientIDs[SnappingClient][i] = s_LastSnapID + 1;
			}
		}
	}
	return Count;
}

bool CPoseCharacter::CanModify(CPlayer *pPlayer)
{
	std::string Key(Server()->ClientName(pPlayer->GetCID()));
	bool Exists = HasPose(pPlayer);

	if(!Exists)
		return true;

	auto &Pose = s_PoseMap[Key];

	bool validTimeoutCode = str_comp(Pose.m_aTimeoutCode, pPlayer->m_TimeoutCode) != 0 && Pose.m_aTimeoutCode[0] != 0;
	char aBuf[48];
	Server()->GetClientAddr(pPlayer->GetCID(), aBuf, NETADDR_MAXSTRSIZE);
	bool validAddress = str_comp(Pose.m_aAddr, aBuf) != 0;

	if(Exists && (validTimeoutCode || validAddress))
	{
		GameServer()->SendChatTarget(pPlayer->GetCID(), g_Config.m_TrNoPermission);
		return false;
	}

	return true;
}

void CPoseCharacter::Init(CGameWorld *pGameWorld)
{
	s_pGameWorld = pGameWorld;
	mem_zero(s_SnapCached, sizeof(s_SnapCached));
	mem_zero(s_FakeClientIDs, sizeof(s_FakeClientIDs));
	mem_zero(s_SnapCache, sizeof(s_SnapCache));

	for(auto &ID : s_FakeEntityIDs)
		ID = Server()->SnapNewID();
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
		auto &Pose = s_PoseMap[Key];
		int BodyHash = HashCoordinate(Pose.m_Core.m_X, Pose.m_Core.m_Y);
		int HookHash = HashCoordinate(Pose.m_Core.m_HookX, Pose.m_Core.m_HookY);
		s_SpatialMap[BodyHash].erase(Key);
		s_SpatialMap[HookHash].erase(Key);
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
		auto &Pose = s_PoseMap[Key];
		int BodyHash = HashCoordinate(Pose.m_Core.m_X, Pose.m_Core.m_Y);
		int HookHash = HashCoordinate(Pose.m_Core.m_HookX, Pose.m_Core.m_HookY);
		s_SpatialMap[BodyHash].erase(Key);
		s_SpatialMap[HookHash].erase(Key);
		s_PoseMap.erase(Key);
		return true;
	}

	return false;
}

bool CPoseCharacter::MovePose(const char *pName, int X, int Y)
{
	std::string Key(pName);
	if(s_PoseMap.count(Key))
	{
		auto &Pose = s_PoseMap[Key];
		int BodyHash = HashCoordinate(Pose.m_Core.m_X, Pose.m_Core.m_Y);
		int HookHash = HashCoordinate(Pose.m_Core.m_HookX, Pose.m_Core.m_HookY);
		s_SpatialMap[BodyHash].erase(Key);
		s_SpatialMap[HookHash].erase(Key);
		Pose.m_Core.m_X += X;
		Pose.m_Core.m_Y += Y;
		Pose.m_Core.m_HookX += X;
		Pose.m_Core.m_HookY += Y;
		BodyHash = HashCoordinate(Pose.m_Core.m_X, Pose.m_Core.m_Y);
		HookHash = HashCoordinate(Pose.m_Core.m_HookX, Pose.m_Core.m_HookY);
		s_SpatialMap[BodyHash].insert(Key);
		s_SpatialMap[HookHash].insert(Key);
		return true;
	}

	return false;
}

const CPoseCharacter *CPoseCharacter::FindPoseByName(const char *pName)
{
	std::string Key(pName);
	if(s_PoseMap.count(Key))
		return &s_PoseMap[Key];

	return NULL;
}

int CPoseCharacter::HashCoordinate(int X, int Y, int OffsetX, int OffsetY)
{
	return ((short)(X / 1000) + OffsetX) + ((short)(Y / 800) + OffsetY) << 16;
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
		GameServer()->SendChatTarget(pPlayer->GetCID(), g_Config.m_TrTooMany);
		return false;
	}

	if(pPlayer->GetCharacter())
	{
		if(pPlayer->GetCharacter()->m_FreezeTime)
		{
			GameServer()->SendChatTarget(pPlayer->GetCID(), g_Config.m_TrFrozen);
			return false;
		}

		vec2 Pos = pPlayer->GetCharacter()->m_Pos;
		if(Pos.x < 200.0f || Pos.y < 200.0f || Pos.x >= GameServer()->Collision()->GetWidth() * 32 - 200.0f || Pos.y >= GameServer()->Collision()->GetHeight() * 32 - 200.0f)
		{
			GameServer()->SendChatTarget(pPlayer->GetCID(), g_Config.m_TrOutOfBounds);
			return false;
		}

		bool Allowed = true;

		// Better check
		if(CountPosesAround(Pos, Allowed) > g_Config.m_SvCrowdednessCheck)
		{
			GameServer()->SendChatTarget(pPlayer->GetCID(), g_Config.m_TrTooFull);
			return false;
		}

		if(!Allowed)
		{
			GameServer()->SendChatTarget(pPlayer->GetCID(), g_Config.m_TrTooClose);
			return false;
		}
	}

	if(pPlayer->GetCharacter())
	{
		auto &Pose = s_PoseMap[Key];

		if(Exists)
			s_AddressCount[Pose.m_aAddr]--;

		Pose.WriteCharacter(pPlayer->GetCharacter());
		Pose.WritePlayer(pPlayer);
		s_AddressCount[Pose.m_aAddr]++;
		Pose.m_Init = true;

		int BodyHash = HashCoordinate(Pose.m_Core.m_X, Pose.m_Core.m_Y);
		int HookHash = HashCoordinate(Pose.m_Core.m_HookX, Pose.m_Core.m_HookY);
		s_SpatialMap[BodyHash].insert(Key);
		s_SpatialMap[HookHash].insert(Key);
	}

	return true;
}

void CPoseCharacter::ChatPosesByIP(int ClientID, const char *pIP)
{
	for(auto &Pose : s_PoseMap)
	{
		if(str_comp(Pose.second.m_aAddr, pIP) == 0)
		{
			GameServer()->SendChatTarget(ClientID, Pose.first.c_str());
		}
	}
}

bool CPoseCharacter::PoseWithName(CPlayer *pPlayer, const char *pName)
{
	char aSafeName[16];
	str_copy(aSafeName, pName, sizeof(aSafeName));
	std::string Key(aSafeName);

	if(s_PoseMap.count(Key))
		return false;

	if(pPlayer->GetCharacter())
	{
		auto &Pose = s_PoseMap[Key];

		Pose.WriteCharacter(pPlayer->GetCharacter());

		StrToInts(&Pose.m_ClientInfo.m_Name0, 4, pName);
		StrToInts(&Pose.m_ClientInfo.m_Clan0, 3, pName);
		Pose.m_ClientInfo.m_Country = Server()->ClientCountry(pPlayer->GetCID());
		StrToInts(&Pose.m_ClientInfo.m_Skin0, 6, pPlayer->m_TeeInfos.m_SkinName);
		Pose.m_ClientInfo.m_UseCustomColor = pPlayer->m_TeeInfos.m_UseCustomColor;
		Pose.m_ClientInfo.m_ColorBody = pPlayer->m_TeeInfos.m_ColorBody;
		Pose.m_ClientInfo.m_ColorFeet = pPlayer->m_TeeInfos.m_ColorFeet;

		Server()->GetClientAddr(pPlayer->GetCID(), Pose.m_aAddr, NETADDR_MAXSTRSIZE);
		str_copy(Pose.m_aTimeoutCode, pPlayer->m_TimeoutCode, 64);

		s_AddressCount[Pose.m_aAddr]++;
		Pose.m_Init = true;
		int BodyHash = HashCoordinate(Pose.m_Core.m_X, Pose.m_Core.m_Y);
		int HookHash = HashCoordinate(Pose.m_Core.m_HookX, Pose.m_Core.m_HookY);
		s_SpatialMap[BodyHash].insert(Key);
		s_SpatialMap[HookHash].insert(Key);
	}

	return true;
}

int CPoseCharacter::FindIDFor(int SnappingClient)
{
	int Result = -1;
	uint32_t SmallestSnapID = UINT32_MAX;
	for(int i = 1; i < FAKE_MAX_CLIENTS; ++i)
	{
		// find oldest one
		if(!s_SnapCache[SnappingClient][i].m_Exists && s_FakeClientIDs[SnappingClient][i] < SmallestSnapID)
		{
			SmallestSnapID = s_FakeClientIDs[SnappingClient][i];
			Result = i;
		}
	}
	return Result;
}

bool CPoseCharacter::IsCurrent(int SnappingClient, int FakeID, void *pID)
{
	if(FakeID == 0)
		return false;
	if(s_SnapCache[SnappingClient][FakeID].m_pID == pID)
		return true;

	return false;
}

CPoseCharacter::CPoseCharacter()
{
	m_Init = false;
	mem_zero(m_ClientPoseMap, sizeof(m_ClientPoseMap));
}

CPoseCharacter::~CPoseCharacter()
{
}

int CPoseCharacter::NetworkClipped(int SnappingClient, vec2 From, vec2 To)
{
	if(SnappingClient == -1)
		return false;

	vec2 ShowDistance;
	ShowDistance = vec2(g_Config.m_SvPosesShowDistanceX, g_Config.m_SvPosesShowDistanceY);

	vec2 ViewPos = GameServer()->m_apPlayers[SnappingClient]->m_ViewPos;
	vec2 TL = vec2(ViewPos.x + ShowDistance.x, ViewPos.y + ShowDistance.y);
	vec2 TR = vec2(ViewPos.x + ShowDistance.x, ViewPos.y - ShowDistance.y);
	vec2 BL = vec2(ViewPos.x - ShowDistance.x, ViewPos.y + ShowDistance.y);
	vec2 BR = vec2(ViewPos.x - ShowDistance.x, ViewPos.y - ShowDistance.y);

	// not clipped if endpoints already in view rect
	if((absolute(ViewPos.x - From.x) <= ShowDistance.x && absolute(ViewPos.y - From.y) <= ShowDistance.y) ||
		(absolute(ViewPos.x - To.x) <= ShowDistance.x && absolute(ViewPos.y - To.y) <= ShowDistance.y))
		return false;

	auto q1 = From;
	auto q2 = To;

	auto SegmentIntersect = [q1, q2](vec2 p1, vec2 p2) -> bool {
		return (((q1.x - p1.x) * (p2.y - p1.y) - (q1.y - p1.y) * (p2.x - p1.x)) * ((q2.x - p1.x) * (p2.y - p1.y) - (q2.y - p1.y) * (p2.x - p1.x)) < 0) &&
		       (((p1.x - q1.x) * (q2.y - q1.y) - (p1.y - q1.y) * (q2.x - q1.x)) * ((p2.x - q1.x) * (q2.y - q1.y) - (p2.y - q1.y) * (q2.x - q1.x)) < 0);
	};

	// not clipped if line intersect with any edge of the view rect
	if(SegmentIntersect(TL, TR) || SegmentIntersect(BL, BR) || SegmentIntersect(TL, BL) || SegmentIntersect(TR, BR))
		return false;

	return true;
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

	for(const auto &Pose : s_PoseMap)
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
	mem_zero(s_SnapCached, sizeof(s_SnapCached));

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

	size_t NumPoses = 0;
	io_read(File, &NumPoses, sizeof(size_t));

	for(size_t i = 0; i < NumPoses; ++i)
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
		int BodyHash = HashCoordinate(Pose.m_Core.m_X, Pose.m_Core.m_Y);
		int HookHash = HashCoordinate(Pose.m_Core.m_HookX, Pose.m_Core.m_HookY);
		s_SpatialMap[BodyHash].insert(Key);
		s_SpatialMap[HookHash].insert(Key);
	}

	io_close(File);
}

void CPoseCharacter::Snap(int SnappingClient)
{
	int _id = 0;
	if(SnappingClient > -1 && !Server()->Translate(_id, SnappingClient))
		return;

	vec2 Pos = vec2(m_Core.m_X, m_Core.m_Y);
	vec2 HookPos = vec2(m_Core.m_HookX, m_Core.m_HookY);
	if(g_Config.m_SvPosesHookLimit >= 0 && distance(Pos, HookPos) > g_Config.m_SvPosesHookLimit)
		HookPos = Pos + normalize(HookPos - Pos) * g_Config.m_SvPosesHookLimit;

	if(m_Core.m_HookState > HOOK_IDLE)
	{
		if(NetworkClipped(SnappingClient, Pos, HookPos))
			return;
	}
	else
	{
		if(NetworkClipped(SnappingClient, Pos, Pos))
			return;
	}

	int ID = m_ClientPoseMap[SnappingClient];
	if(!IsCurrent(SnappingClient, ID, this))
		ID = FindIDFor(SnappingClient);
	m_ClientPoseMap[SnappingClient] = ID;

	if(ID <= 0)
		return;

	auto *pCache = &s_SnapCache[SnappingClient][ID];

	int NeedSkip = pCache->m_pID != this && s_FakeClientIDs[SnappingClient][ID] == s_LastSnapID;
	pCache->m_pExpectedID = this;

	pCache->m_NeedSkip = NeedSkip;
	pCache->m_Laser.m_X = (int)m_Core.m_X;
	pCache->m_Laser.m_Y = (int)m_Core.m_Y;
	pCache->m_Laser.m_FromX = (int)m_Core.m_X;
	pCache->m_Laser.m_FromY = (int)m_Core.m_Y;

	pCache->m_Char.m_X = m_Core.m_X;
	pCache->m_Char.m_Y = m_Core.m_Y;
	pCache->m_Char.m_VelX = 0;
	pCache->m_Char.m_VelY = 0;
	pCache->m_Char.m_Angle = m_Core.m_Angle;
	pCache->m_Char.m_Direction = 0;
	pCache->m_Char.m_Jumped = 0;
	pCache->m_Char.m_HookState = m_Core.m_HookState > HOOK_IDLE ? HOOK_GRABBED : HOOK_IDLE;
	pCache->m_Char.m_HookTick = 0;

	pCache->m_Char.m_HookX = int(HookPos.x);
	pCache->m_Char.m_HookY = int(HookPos.y);
	pCache->m_Char.m_HookDx = 0;
	pCache->m_Char.m_HookDy = 0;
	pCache->m_Char.m_Emote = m_EmoteType;
	pCache->m_Char.m_HookedPlayer = -1;
	pCache->m_Char.m_AttackTick = 0;
	pCache->m_Char.m_Weapon = m_Weapon;
	pCache->m_Char.m_AmmoCount = 0;
	pCache->m_Char.m_Health = 10;
	pCache->m_Char.m_Armor = 0;
	pCache->m_Char.m_PlayerFlags = 0;

	pCache->m_DDNetChar.m_Flags = 0;
	pCache->m_DDNetChar.m_Flags |= CHARACTERFLAG_SOLO;
	pCache->m_DDNetChar.m_Flags |= CHARACTERFLAG_NO_COLLISION;
	pCache->m_DDNetChar.m_Flags |= CHARACTERFLAG_NO_HOOK;
	pCache->m_DDNetChar.m_Flags |= CHARACTERFLAG_NO_HAMMER_HIT;
	pCache->m_DDNetChar.m_Flags |= CHARACTERFLAG_NO_GRENADE_HIT;
	pCache->m_DDNetChar.m_Flags |= CHARACTERFLAG_NO_LASER_HIT;
	pCache->m_DDNetChar.m_Flags |= CHARACTERFLAG_NO_SHOTGUN_HIT;
	pCache->m_DDNetChar.m_Flags |= CHARACTERFLAG_WEAPON_HAMMER;
	pCache->m_DDNetChar.m_Flags |= CHARACTERFLAG_WEAPON_GUN;
	pCache->m_DDNetChar.m_Flags |= CHARACTERFLAG_WEAPON_SHOTGUN;
	pCache->m_DDNetChar.m_Flags |= CHARACTERFLAG_WEAPON_GRENADE;
	pCache->m_DDNetChar.m_Flags |= CHARACTERFLAG_WEAPON_LASER;

	pCache->m_DDNetChar.m_FreezeEnd = 0;
	pCache->m_DDNetChar.m_Jumps = 1;
	pCache->m_DDNetChar.m_TeleCheckpoint = 0;
	pCache->m_DDNetChar.m_StrongWeakID = 0;

	pCache->m_ClientInfo = m_ClientInfo;

	pCache->m_PlayerInfo.m_Latency = 999;
	int minute = g_Config.m_SvYear / 100;
	int second = g_Config.m_SvYear % 100;
	pCache->m_PlayerInfo.m_Score = -(minute * 60 + second);
	pCache->m_PlayerInfo.m_Local = 0;
	pCache->m_PlayerInfo.m_ClientID = ID;
	pCache->m_PlayerInfo.m_Team = 0;

	pCache->m_Exists = true;
}
