/* CMaNGOS TBC - The Stockade (map 34): Kam Deepfury */

#include "AI/ScriptDevAI/include/sc_common.h"

enum
{
    SPELL_MORTAL_STRIKE           = 12294,
    SPELL_WHIRLWIND               = 1680,
    SPELL_INTIMIDATING_SHOUT      = 5246,

    SAY_AGGRO                     = -1034003,
    SAY_SLAY                      = -1034004,
    SAY_DEATH                     = -1034005,
};

struct boss_kam_deepfuryAI : public ScriptedAI
{
    boss_kam_deepfuryAI(Creature* pCreature) : ScriptedAI(pCreature) { Reset(); }

    uint32 m_uiMortalStrikeTimer;
    uint32 m_uiWhirlwindTimer;
    uint32 m_uiShoutTimer;

    void Reset() override
    {
        m_uiMortalStrikeTimer = urand(8000, 12000);
        m_uiWhirlwindTimer = urand(14000, 19000);
        m_uiShoutTimer = urand(20000, 26000);
    }

    void Aggro(Unit* pWho) override { DoScriptText(SAY_AGGRO, m_creature); }
    void KilledUnit(Unit* pVictim) override { DoScriptText(SAY_SLAY, m_creature); }
    void JustDied(Unit* pKiller) override { DoScriptText(SAY_DEATH, m_creature); }

    void UpdateAI(const uint32 uiDiff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->GetVictim())
            return;

        if (m_uiMortalStrikeTimer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature->GetVictim(), SPELL_MORTAL_STRIKE) == CAST_OK)
                m_uiMortalStrikeTimer = urand(8000, 12000);
        }
        else
            m_uiMortalStrikeTimer -= uiDiff;

        if (m_uiWhirlwindTimer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_WHIRLWIND) == CAST_OK)
                m_uiWhirlwindTimer = urand(14000, 19000);
        }
        else
            m_uiWhirlwindTimer -= uiDiff;

        if (m_uiShoutTimer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_INTIMIDATING_SHOUT) == CAST_OK)
                m_uiShoutTimer = urand(20000, 26000);
        }
        else
            m_uiShoutTimer -= uiDiff;

        DoMeleeAttackIfReady();
    }
};

UnitAI* GetAI_boss_kam_deepfury(Creature* pCreature)
{
    return new boss_kam_deepfuryAI(pCreature);
}

void AddSC_boss_kam_deepfury()
{
    Script* pNewScript = new Script;
    pNewScript->Name = "boss_kam_deepfury";
    pNewScript->GetAI = &GetAI_boss_kam_deepfury;
    pNewScript->RegisterSelf();
}