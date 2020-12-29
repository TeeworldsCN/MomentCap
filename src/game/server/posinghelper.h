#ifndef GAME_SERVER_POSINGHELPER_H
#define GAME_SERVER_POSINGHELPER_H

#include <game/generated/protocol.h>
#include <game/generated/server_data.h>
#include <game/server/entity.h>

#include <game/gamecore.h>

enum
{
	MAX_POSES = 1024
};

class CPoseCharacter : public CEntity
{
public:
	CPoseCharacter(CGameWorld *pWorld);

	static void ResetClientIDs();
	static int FindIDFor(int SnappingClient);
	static bool IsCurrent(int SnappingClient, int FakeID);
	static void StepSnapID() { s_LastSnapID++; }

	virtual void Reset() {}
	virtual void Destroy() {}
	virtual void Tick() {}
	virtual void TickDefered() {}
	virtual void TickPaused() {}
	virtual void Snap(int SnappingClient);
	virtual int NetworkClipped(int SnappingClient);
	virtual int NetworkClipped(int SnappingClient, vec2 CheckPos);

	void WriteCharacter(CCharacter *pCharacter);
	void WritePlayer(CPlayer *pPlayer);

private:
	static short s_FakeClientIDs[MAX_CLIENTS][MAX_CLIENTS];
	static short s_LastSnapID;

	uint8_t m_ClientPoseMap[MAX_CLIENTS];

	int m_Weapon;
	int m_EmoteType;
	CNetObj_CharacterCore m_Core;
	CNetObj_ClientInfo m_ClientInfo;

	void SnapCharacter(int SnappingClient, int ID);
	void SnapPlayer(int SnappingClient, int ID);
};

#endif