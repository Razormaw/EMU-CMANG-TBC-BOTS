#include "AI/ScriptDevAI/include/sc_common.h"

enum { EMOTE_AGGRO = -1070009, EMOTE_DEATH = -1070010, SPELL_KNOCK_AWAY = 10101, SPELL_ENRAGE = 8599, NPC_ANCIENT_STONE_KEEPER = 7206 };

struct boss_ancient_stone_keeperAI : public ScriptedAI
{
    boss_ancient_stone_keeperAI(Creature* pCreature) : ScriptedAI(pCreature) { Reset(); }
    uint32 m_uiKnockTimer; bool m_bEnraged;
    void Reset() override { m_uiKnockTimer = urand(10000, 15000); m_bEnraged = false; }
    void Aggro(Unit* pWho) override { DoScriptText(EMOTE_AGGRO, m_creature); }
    void JustDied(Unit* pKiller) override { DoScriptText(EMOTE_DEATH, m_creature); }
    void UpdateAI(const uint32 uiDiff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->GetVictim()) return;
        if (!m_bEnraged && m_creature->GetHealthPercent() < 25.0f)
        { if (DoCastSpellIfCan(m_creature, SPELL_ENRAGE) == CAST_OK) m_bEnraged = true; }
        if (m_uiKnockTimer < uiDiff)
        { if (DoCastSpellIfCan(m_creature->GetVictim(), SPELL_KNOCK_AWAY) == CAST_OK) m_uiKnockTimer = urand(12000, 17000); }
        else m_uiKnockTimer -= uiDiff;
        DoMeleeAttackIfReady();
    }
};
UnitAI* GetAI_boss_ancient_stone_keeper(Creature* pCreature) { return new boss_ancient_stone_keeperAI(pCreature); }
void AddSC_boss_ancient_stone_keeper() { Script* pNewScript = new Script; pNewScript->Name = "boss_ancient_stone_keeper"; pNewScript->GetAI = &GetAI_boss_ancient_stone_keeper; pNewScript->RegisterSelf(); }

