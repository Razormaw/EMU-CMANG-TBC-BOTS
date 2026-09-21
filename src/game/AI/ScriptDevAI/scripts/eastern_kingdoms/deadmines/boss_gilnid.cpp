#include "AI/ScriptDevAI/include/sc_common.h"
enum { SAY_AGGRO = -1036011, SAY_SLAY = -1036012, SAY_DEATH = -1036013, SPELL_STOMP = 5589, SPELL_ENRAGE = 8599, NPC_GILNID = 1763 };
struct boss_gilnidAI : public ScriptedAI
{
    boss_gilnidAI(Creature* pCreature) : ScriptedAI(pCreature) { Reset(); }
    uint32 m_uiStompTimer; bool m_bEnraged;
    void Reset() override { m_uiStompTimer = urand(10000, 15000); m_bEnraged = false; }
    void Aggro(Unit* pWho) override { DoScriptText(SAY_AGGRO, m_creature); }
    void KilledUnit(Unit* pVictim) override { DoScriptText(SAY_SLAY, m_creature); }
    void JustDied(Unit* pKiller) override { DoScriptText(SAY_DEATH, m_creature); }
    void UpdateAI(const uint32 uiDiff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->GetVictim()) return;
        if (!m_bEnraged && m_creature->GetHealthPercent() < 30.0f) { if (DoCastSpellIfCan(m_creature, SPELL_ENRAGE) == CAST_OK) m_bEnraged = true; }
        if (m_uiStompTimer < uiDiff) { if (DoCastSpellIfCan(m_creature, SPELL_STOMP) == CAST_OK) m_uiStompTimer = urand(12000, 18000); } else m_uiStompTimer -= uiDiff;
        DoMeleeAttackIfReady();
    }
};
UnitAI* GetAI_boss_gilnid(Creature* pCreature) { return new boss_gilnidAI(pCreature); }
void AddSC_boss_gilnid() { Script* pNewScript = new Script; pNewScript->Name = "boss_gilnid"; pNewScript->GetAI = &GetAI_boss_gilnid; pNewScript->RegisterSelf(); }

