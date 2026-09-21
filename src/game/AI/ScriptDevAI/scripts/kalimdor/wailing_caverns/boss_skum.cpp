#include "AI/ScriptDevAI/include/sc_common.h"

enum { EMOTE_AGGRO = -1043031, EMOTE_DEATH = -1043032, SPELL_REND = 11572, SPELL_ENRAGE = 8599, NPC_SKUM = 3674 };

struct boss_skumAI : public ScriptedAI
{
    boss_skumAI(Creature* pCreature) : ScriptedAI(pCreature) { Reset(); }
    uint32 m_uiRendTimer; bool m_bEnraged;
    void Reset() override { m_uiRendTimer = urand(6000, 10000); m_bEnraged = false; }
    void Aggro(Unit* pWho) override { DoScriptText(EMOTE_AGGRO, m_creature); }
    void JustDied(Unit* pKiller) override { DoScriptText(EMOTE_DEATH, m_creature); }
    void UpdateAI(const uint32 uiDiff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->GetVictim()) return;
        if (!m_bEnraged && m_creature->GetHealthPercent() < 30.0f)
        { if (DoCastSpellIfCan(m_creature, SPELL_ENRAGE) == CAST_OK) m_bEnraged = true; }
        if (m_uiRendTimer < uiDiff)
        { if (DoCastSpellIfCan(m_creature->GetVictim(), SPELL_REND) == CAST_OK) m_uiRendTimer = urand(8000, 12000); }
        else m_uiRendTimer -= uiDiff;
        DoMeleeAttackIfReady();
    }
};
UnitAI* GetAI_boss_skum(Creature* pCreature) { return new boss_skumAI(pCreature); }
void AddSC_boss_skum() { Script* pNewScript = new Script; pNewScript->Name = "boss_skum"; pNewScript->GetAI = &GetAI_boss_skum; pNewScript->RegisterSelf(); }

