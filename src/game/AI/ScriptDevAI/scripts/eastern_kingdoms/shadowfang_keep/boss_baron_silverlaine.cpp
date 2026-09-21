#include "AI/ScriptDevAI/include/sc_common.h"
enum { SAY_AGGRO = -1033022, SAY_SLAY = -1033023, SAY_DEATH = -1033024, SPELL_SHADOW_BOLT = 12739, SPELL_FEAR = 5782, NPC_BARON_SILVERLAINE = 3887 };
struct boss_baron_silverlaineAI : public ScriptedAI
{
    boss_baron_silverlaineAI(Creature* pCreature) : ScriptedAI(pCreature) { Reset(); }
    uint32 m_uiBoltTimer, m_uiFearTimer;
    void Reset() override { m_uiBoltTimer = urand(5000, 8000); m_uiFearTimer = urand(12000, 17000); m_attackDistance = 20.0f; }
    void Aggro(Unit* pWho) override { DoScriptText(SAY_AGGRO, m_creature); }
    void KilledUnit(Unit* pVictim) override { DoScriptText(SAY_SLAY, m_creature); }
    void JustDied(Unit* pKiller) override { DoScriptText(SAY_DEATH, m_creature); }
    void UpdateAI(const uint32 uiDiff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->GetVictim()) return;
        if (m_uiBoltTimer < uiDiff) { if (DoCastSpellIfCan(m_creature->GetVictim(), SPELL_SHADOW_BOLT) == CAST_OK) m_uiBoltTimer = urand(5000, 8000); } else m_uiBoltTimer -= uiDiff;
        if (m_uiFearTimer < uiDiff) { if (DoCastSpellIfCan(m_creature->GetVictim(), SPELL_FEAR) == CAST_OK) m_uiFearTimer = urand(14000, 19000); } else m_uiFearTimer -= uiDiff;
        DoMeleeAttackIfReady();
    }
};
UnitAI* GetAI_boss_baron_silverlaine(Creature* pCreature) { return new boss_baron_silverlaineAI(pCreature); }
void AddSC_boss_baron_silverlaine() { Script* pNewScript = new Script; pNewScript->Name = "boss_baron_silverlaine"; pNewScript->GetAI = &GetAI_boss_baron_silverlaine; pNewScript->RegisterSelf(); }

