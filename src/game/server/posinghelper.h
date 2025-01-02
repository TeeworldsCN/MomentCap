#ifndef GAME_SERVER_POSINGHELPER_H
#define GAME_SERVER_POSINGHELPER_H

#include <game/generated/protocol.h>
#include <game/generated/server_data.h>
#include <game/server/entity.h>

#include <game/gamecore.h>
#include <set>
#include <unordered_map>
#include <unordered_set>

struct SPoseSnapCache
{
	void *m_pID;
	void *m_pExpectedID;
	bool m_Exists;
	bool m_NeedSkip;
	CNetObj_Character m_Char;
	CNetObj_DDNetCharacter m_DDNetChar;
	CNetObj_Laser m_Laser;
	CNetObj_ClientInfo m_ClientInfo;
	CNetObj_PlayerInfo m_PlayerInfo;
	CNetObj_DDNetPlayer m_DDNetPlayer;
};

class CPoseCharacter
{
public:
	CPoseCharacter();
	~CPoseCharacter();

	static class CGameWorld *GameWorld() { return s_pGameWorld; }
	static class CGameContext *GameServer() { return GameWorld()->GameServer(); }
	static class IServer *Server() { return GameWorld()->Server(); }
	static int SnapPoses(int SnappingClient, bool AsSpec, bool NewSnap);
	static int CountPosesOfSpace(vec2 Pos, int SpatialKey, bool &Allowed);
	static int CountPosesAround(vec2 Pos, bool &Allowed);
	static void SnapPosesOfSpace(int SnappingClient, int SpatialKey);

	static const CPoseCharacter *FindPoseByName(const char *pName);

	static bool CanModify(CPlayer *pPlayer);
	static bool HasPose(CPlayer *pPlayer);
	static bool RemovePose(CPlayer *pPlayer);
	static bool Pose(CPlayer *pPlayer);

	// auth tool
	static bool RemovePoseByName(const char *pName);
	static bool MovePose(const char *pName, int X, int Y);
	static bool PoseWithName(CPlayer *pPlayer, const char *pName);
	static void ChatPosesByIP(int ClientID, const char *pName);
	// static bool PoseHookLengthDelta(const char *pName, int Delta, int Asker);
	static void SavePoses();
	static void LoadPoses();

	static const CPoseCharacter *ClosestPose(vec2 Pos, float Radius);
	static int Count() { return s_PoseMap.size(); }

	static void Init(CGameWorld *pGameWorld);
	static int FindIDFor(int SnappingClient);
	static bool IsCurrent(int SnappingClient, int FakeID, void *pID);
	static void StepSnapID() { s_LastSnapID++; }

	vec2 Position() const { return vec2(m_Core.m_X, m_Core.m_Y); }
	float Distance(vec2 Pos) const { return distance(Pos, vec2(m_Core.m_X, m_Core.m_Y)); }
	void Snap(int SnappingClient);
	static int NetworkClipped(int SnappingClient, vec2 From, vec2 To);

	CNetObj_ClientInfo m_ClientInfo;
	char m_aAddr[NETADDR_MAXSTRSIZE];
	char m_aTimeoutCode[64];

private:
	static CGameWorld *s_pGameWorld;

	static bool s_SnapCached[MAX_CLIENTS];
	static SPoseSnapCache s_SnapCache[MAX_CLIENTS][FAKE_MAX_CLIENTS];
	static int s_FakeEntityIDs[FAKE_MAX_CLIENTS];

	static uint32_t s_FakeClientIDs[MAX_CLIENTS][FAKE_MAX_CLIENTS];
	static uint32_t s_LastSnapID;
	static std::unordered_map<std::string, CPoseCharacter> s_PoseMap;
	static std::unordered_map<int, std::set<std::string>> s_SpatialMap;
	static std::unordered_set<std::string> s_PoseSnapCache;
	static std::unordered_map<std::string, int> s_AddressCount;

	static int HashCoordinate(int X, int Y, int OffsetX = 0, int OffsetY = 0);

	int m_ClientPoseMap[MAX_CLIENTS];

	bool m_Init;
	int m_Weapon;
	int m_EmoteType;
	CNetObj_CharacterCore m_Core;

	void SnapCharacter(int SnappingClient, int ID);
	void SnapPlayer(int SnappingClient, int ID);

	void WriteCharacter(CCharacter *pCharacter);
	void WritePlayer(CPlayer *pPlayer);
};

#endif