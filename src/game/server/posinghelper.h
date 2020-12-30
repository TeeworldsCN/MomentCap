#ifndef GAME_SERVER_POSINGHELPER_H
#define GAME_SERVER_POSINGHELPER_H

#include <game/generated/protocol.h>
#include <game/generated/server_data.h>
#include <game/server/entity.h>

#include <game/gamecore.h>
#include <unordered_map>

class CPoseCharacter
{
public:
	CPoseCharacter();

	static class CGameWorld *GameWorld() { return s_pGameWorld; }
	static class CGameContext *GameServer() { return GameWorld()->GameServer(); }
	static class IServer *Server() { return GameWorld()->Server(); }
	static void SnapPoses(int SnappingClient);

	static bool HasPose(CPlayer *pPlayer);
	static void RemovePose(CPlayer *pPlayer);
	static void Pose(CPlayer *pPlayer);

	static void Init(CGameWorld *pGameWorld);
	static int FindIDFor(int SnappingClient);
	static bool IsCurrent(int SnappingClient, int FakeID);
	static void StepSnapID() { s_LastSnapID++; }

	void Snap(int SnappingClient);
	int NetworkClipped(int SnappingClient);
	int NetworkClipped(int SnappingClient, vec2 CheckPos);

private:
	static CGameWorld *s_pGameWorld;

	static short s_FakeClientIDs[MAX_CLIENTS][FAKE_MAX_CLIENTS];
	static short s_LastSnapID;
	static std::unordered_map<std::string, CPoseCharacter> s_PoseMap;
	static std::unordered_map<std::string, int> s_AddressCount;

	uint8_t m_ClientPoseMap[MAX_CLIENTS];

	bool m_Init;
	vec2 m_Pos;
	int m_Weapon;
	int m_EmoteType;
	CNetObj_CharacterCore m_Core;
	CNetObj_ClientInfo m_ClientInfo;

	char aAddr[NETADDR_MAXSTRSIZE];
	char aTimeoutCode[64];

	void SnapCharacter(int SnappingClient, int ID);
	void SnapPlayer(int SnappingClient, int ID);

	void WriteCharacter(CCharacter *pCharacter);
	void WritePlayer(CPlayer *pPlayer);
};

#endif