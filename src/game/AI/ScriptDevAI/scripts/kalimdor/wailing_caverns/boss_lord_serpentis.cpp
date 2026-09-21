#include "AI/ScriptDevAI/include/sc_common.h"

enum { SAY_AGGRO = -1043026, SAY_SLAY = -1043027, SAY_DEATH = -1043028, SPELL_STOMP = 5589, SPELL_ENRAGE = 8599, NPC_LORD_SERPENTIS = 3673 };

struct boss_lord_serpentisAI : public ScriptedAI
{
    boss_lord_serpentisAI(Creature* pCreature) : ScriptedAI(pCreature) { Reset(); }
    uint32 m_uiStompTimer; bool m_bEnraged;
    void Reset() override { m_uiStompTimer = urand(10000, 15000); m_bEnraged = false; }
    void Aggro(Unit* pWho) override { DoScriptText(SAY_AGGRO, m_creature); }
    void KilledUnit(Unit* pVictim) override { DoScriptText(SAY_SLAY, m_creature); }
    void JustDied(Unit* pKiller) override { DoScriptText(SAY_DEATH, m_creature); }
    void UpdateAI(const uint32 uiDiff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->GetVictim()) return;
        if (!m_bEnraged && m_creature->GetHealthPercent() < 30.0f)
        { if (DoCastSpellIfCan(m_creature, SPELL_ENRAGE) == CAST_OK) m_bEnraged = true; }
        if (m_uiStompTimer < uiDiff)
        { if (DoCastSpellIfCan(m_creature, SPELL_STOMP) == CAST_OK) m_uiStompTimer = urand(12000, 18000); }
        else m_uiStompTimer -= uiDiff;
        DoMeleeAttackIfReady();
    }
};
UnitAI* GetAI_boss_lord_serpentis(Creature* pCreature) { return new boss_lord_serpentisAI(pCreature); }
void AddSC_boss_lord_serpentis() { Script* pNewScript = new Script; pNewScript->Name = "boss_lord_serpentis"; pNewScript->GetAI = &GetAI_boss_lord_serpentis; pNewScript->RegisterSelf(); }

