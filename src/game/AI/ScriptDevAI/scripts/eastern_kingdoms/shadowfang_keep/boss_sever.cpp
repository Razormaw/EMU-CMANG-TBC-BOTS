#include "AI/ScriptDevAI/include/sc_common.h"
enum { EMOTE_AGGRO = -1033040, EMOTE_DEATH = -1033041, SPELL_REND = 11572, SPELL_KNOCK_AWAY = 10101, NPC_SEVER = 14682 };
struct boss_severAI : public ScriptedAI
{
    boss_severAI(Creature* pCreature) : ScriptedAI(pCreature) { Reset(); }
    uint32 m_uiRendTimer, m_uiKnockTimer;
    void Reset() override { m_uiRendTimer = urand(6000, 10000); m_uiKnockTimer = urand(12000, 16000); }
    void Aggro(Unit* pWho) override { DoScriptText(EMOTE_AGGRO, m_creature); }
    void JustDied(Unit* pKiller) override { DoScriptText(EMOTE_DEATH, m_creature); }
    void UpdateAI(const uint32 uiDiff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->GetVictim()) return;
        if (m_uiRendTimer < uiDiff) { if (DoCastSpellIfCan(m_creature->GetVictim(), SPELL_REND) == CAST_OK) m_uiRendTimer = urand(8000, 12000); } else m_uiRendTimer -= uiDiff;
        if (m_uiKnockTimer < uiDiff) { if (DoCastSpellIfCan(m_creature->GetVictim(), SPELL_KNOCK_AWAY) == CAST_OK) m_uiKnockTimer = urand(14000, 18000); } else m_uiKnockTimer -= uiDiff;
        DoMeleeAttackIfReady();
    }
};
UnitAI* GetAI_boss_sever(Creature* pCreature) { return new boss_severAI(pCreature); }
void AddSC_boss_sever() { Script* pNewScript = new Script; pNewScript->Name = "boss_sever"; pNewScript->GetAI = &GetAI_boss_sever; pNewScript->RegisterSelf(); }

