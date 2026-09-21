#include "AI/ScriptDevAI/include/sc_common.h"
enum { SAY_AGGRO = -1036017, SAY_SLAY = -1036018, SAY_ENRAGE = -1036019, SAY_DEATH = -1036020, SPELL_REND = 11572, SPELL_HAMSTRING = 1715, SPELL_ENRAGE = 8599, NPC_EDWIN_VANCLEF = 639 };
struct boss_edwin_vanclefAI : public ScriptedAI
{
    boss_edwin_vanclefAI(Creature* pCreature) : ScriptedAI(pCreature) { Reset(); }
    uint32 m_uiRendTimer, m_uiHamstringTimer; bool m_bEnraged;
    void Reset() override { m_uiRendTimer = urand(6000, 10000); m_uiHamstringTimer = urand(9000, 14000); m_bEnraged = false; }
    void Aggro(Unit* pWho) override { DoScriptText(SAY_AGGRO, m_creature); }
    void KilledUnit(Unit* pVictim) override { DoScriptText(SAY_SLAY, m_creature); }
    void JustDied(Unit* pKiller) override { DoScriptText(SAY_DEATH, m_creature); }
    void UpdateAI(const uint32 uiDiff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->GetVictim()) return;
        if (!m_bEnraged && m_creature->GetHealthPercent() < 25.0f) { if (DoCastSpellIfCan(m_creature, SPELL_ENRAGE) == CAST_OK) { DoScriptText(SAY_ENRAGE, m_creature); m_bEnraged = true; } }
        if (m_uiRendTimer < uiDiff) { if (DoCastSpellIfCan(m_creature->GetVictim(), SPELL_REND) == CAST_OK) m_uiRendTimer = urand(8000, 12000); } else m_uiRendTimer -= uiDiff;
        if (m_uiHamstringTimer < uiDiff) { if (DoCastSpellIfCan(m_creature->GetVictim(), SPELL_HAMSTRING) == CAST_OK) m_uiHamstringTimer = urand(10000, 15000); } else m_uiHamstringTimer -= uiDiff;
        DoMeleeAttackIfReady();
    }
};
UnitAI* GetAI_boss_edwin_vanclef(Creature* pCreature) { return new boss_edwin_vanclefAI(pCreature); }
void AddSC_boss_edwin_vanclef() { Script* pNewScript = new Script; pNewScript->Name = "boss_edwin_vanclef"; pNewScript->GetAI = &GetAI_boss_edwin_vanclef; pNewScript->RegisterSelf(); }

