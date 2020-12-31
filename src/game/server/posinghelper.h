#ifndef GAME_SERVER_POSINGHELPER_H
#define GAME_SERVER_POSINGHELPER_H

#include <game/generated/protocol.h>
#include <game/generated/server_data.h>
#include <game/server/entity.h>

#include <game/gamecore.h>
#include <unordered_map>

struct SPoseSnapCache
{
	bool m_Exists;
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
	static void SnapPoses(int SnappingClient, bool AsSpec, bool NewSnap);

	static bool CanModify(CPlayer *pPlayer);
	static bool HasPose(CPlayer *pPlayer);
	static bool RemovePose(CPlayer *pPlayer);
	static bool RemovePoseByName(const char *pName);
	static bool Pose(CPlayer *pPlayer);
	static void SavePoses();
	static void LoadPoses();
	static const CPoseCharacter *ClosestPose(vec2 Pos, float Radius);
	static int Count() { return s_PoseMap.size(); }

	static void Init(CGameWorld *pGameWorld);
	static int FindIDFor(int SnappingClient);
	static bool IsCurrent(int SnappingClient, int FakeID);
	static void StepSnapID() { s_LastSnapID++; }

	float Distance(vec2 Pos) const { return distance(Pos, vec2(m_Core.m_X, m_Core.m_Y)); }
	void Snap(int SnappingClient);
	int NetworkClipped(int SnappingClient);
	int NetworkClipped(int SnappingClient, vec2 CheckPos);

	CNetObj_ClientInfo m_ClientInfo;
	char m_aAddr[NETADDR_MAXSTRSIZE];
	char m_aTimeoutCode[64];

private:
	static CGameWorld *s_pGameWorld;

	static bool s_SnapCached[MAX_CLIENTS];
	static SPoseSnapCache s_SnapCache[MAX_CLIENTS][FAKE_MAX_CLIENTS];
	static uint8_t s_FakeClientIDs[MAX_CLIENTS][FAKE_MAX_CLIENTS];
	static int s_FakeEntityIDs[FAKE_MAX_CLIENTS];
	static uint8_t s_LastSnapID;
	static std::unordered_map<std::string, CPoseCharacter> s_PoseMap;
	static std::unordered_map<std::string, int> s_AddressCount;

	uint8_t m_ClientPoseMap[MAX_CLIENTS];

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