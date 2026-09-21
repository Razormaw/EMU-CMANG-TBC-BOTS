#include "AI/ScriptDevAI/include/sc_common.h"

enum { EMOTE_AGGRO = -1043029, EMOTE_DEATH = -1043030, SPELL_STOMP = 5589, NPC_KRESH = 3653 };

struct boss_kreshAI : public ScriptedAI
{
    boss_kreshAI(Creature* pCreature) : ScriptedAI(pCreature) { Reset(); }
    uint32 m_uiStompTimer;
    void Reset() override { m_uiStompTimer = urand(12000, 18000); }
    void Aggro(Unit* pWho) override { DoScriptText(EMOTE_AGGRO, m_creature); }
    void JustDied(Unit* pKiller) override { DoScriptText(EMOTE_DEATH, m_creature); }
    void UpdateAI(const uint32 uiDiff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->GetVictim()) return;
        if (m_uiStompTimer < uiDiff)
        { if (DoCastSpellIfCan(m_creature, SPELL_STOMP) == CAST_OK) m_uiStompTimer = urand(14000, 20000); }
        else m_uiStompTimer -= uiDiff;
        DoMeleeAttackIfReady();
    }
};
UnitAI* GetAI_boss_kresh(Creature* pCreature) { return new boss_kreshAI(pCreature); }
void AddSC_boss_kresh() { Script* pNewScript = new Script; pNewScript->Name = "boss_kresh"; pNewScript->GetAI = &GetAI_boss_kresh; pNewScript->RegisterSelf(); }

