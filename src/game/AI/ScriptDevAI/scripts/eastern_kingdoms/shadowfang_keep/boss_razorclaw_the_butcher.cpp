#include "AI/ScriptDevAI/include/sc_common.h"
enum { EMOTE_AGGRO = -1033020, EMOTE_DEATH = -1033021, SPELL_REND = 11572, SPELL_ENRAGE = 8599, NPC_RAZORCLAW = 3886 };
struct boss_razorclaw_the_butcherAI : public ScriptedAI
{
    boss_razorclaw_the_butcherAI(Creature* pCreature) : ScriptedAI(pCreature) { Reset(); }
    uint32 m_uiRendTimer; bool m_bEnraged;
    void Reset() override { m_uiRendTimer = urand(6000, 10000); m_bEnraged = false; }
    void Aggro(Unit* pWho) override { DoScriptText(EMOTE_AGGRO, m_creature); }
    void JustDied(Unit* pKiller) override { DoScriptText(EMOTE_DEATH, m_creature); }
    void UpdateAI(const uint32 uiDiff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->GetVictim()) return;
        if (!m_bEnraged && m_creature->GetHealthPercent() < 30.0f) { if (DoCastSpellIfCan(m_creature, SPELL_ENRAGE) == CAST_OK) m_bEnraged = true; }
        if (m_uiRendTimer < uiDiff) { if (DoCastSpellIfCan(m_creature->GetVictim(), SPELL_REND) == CAST_OK) m_uiRendTimer = urand(8000, 12000); } else m_uiRendTimer -= uiDiff;
        DoMeleeAttackIfReady();
    }
};
UnitAI* GetAI_boss_razorclaw_the_butcher(Creature* pCreature) { return new boss_razorclaw_the_butcherAI(pCreature); }
void AddSC_boss_razorclaw_the_butcher() { Script* pNewScript = new Script; pNewScript->Name = "boss_razorclaw_the_butcher"; pNewScript->GetAI = &GetAI_boss_razorclaw_the_butcher; pNewScript->RegisterSelf(); }

