#include "AI/ScriptDevAI/include/sc_common.h"
enum { SAY_AGGRO = -1036014, SAY_SLAY = -1036015, SAY_DEATH = -1036016, SPELL_REND = 11572, SPELL_HAMSTRING = 1715, NPC_CAPTAIN_GREENSKIN = 647 };
struct boss_captain_greenskinAI : public ScriptedAI
{
    boss_captain_greenskinAI(Creature* pCreature) : ScriptedAI(pCreature) { Reset(); }
    uint32 m_uiRendTimer, m_uiHamstringTimer;
    void Reset() override { m_uiRendTimer = urand(6000, 10000); m_uiHamstringTimer = urand(9000, 14000); }
    void Aggro(Unit* pWho) override { DoScriptText(SAY_AGGRO, m_creature); }
    void KilledUnit(Unit* pVictim) override { DoScriptText(SAY_SLAY, m_creature); }
    void JustDied(Unit* pKiller) override { DoScriptText(SAY_DEATH, m_creature); }
    void UpdateAI(const uint32 uiDiff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->GetVictim()) return;
        if (m_uiRendTimer < uiDiff) { if (DoCastSpellIfCan(m_creature->GetVictim(), SPELL_REND) == CAST_OK) m_uiRendTimer = urand(8000, 12000); } else m_uiRendTimer -= uiDiff;
        if (m_uiHamstringTimer < uiDiff) { if (DoCastSpellIfCan(m_creature->GetVictim(), SPELL_HAMSTRING) == CAST_OK) m_uiHamstringTimer = urand(10000, 15000); } else m_uiHamstringTimer -= uiDiff;
        DoMeleeAttackIfReady();
    }
};
UnitAI* GetAI_boss_captain_greenskin(Creature* pCreature) { return new boss_captain_greenskinAI(pCreature); }
void AddSC_boss_captain_greenskin() { Script* pNewScript = new Script; pNewScript->Name = "boss_captain_greenskin"; pNewScript->GetAI = &GetAI_boss_captain_greenskin; pNewScript->RegisterSelf(); }

