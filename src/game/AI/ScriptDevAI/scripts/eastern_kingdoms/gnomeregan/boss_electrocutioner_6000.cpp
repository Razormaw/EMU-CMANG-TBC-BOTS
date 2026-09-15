/* CMaNGOS TBC - Gnomeregan (map 90): Electrocutioner 6000 */

#include "AI/ScriptDevAI/include/sc_common.h"
#include "gnomeregan.h"

enum
{
    EMOTE_AGGRO               = -1090034,
    EMOTE_DEATH               = -1090035,

    SPELL_SHOCK               = 11084,    // VERIFICADO (wowhead classic): rayo instantáneo
    SPELL_CHAIN_BOLT          = 11085,    // VERIFICADO: rayo en cadena a 3 objetivos
    SPELL_MEGAVOLT            = 11082,    // VERIFICADO en tu cliente: cono frontal
};

struct boss_electrocutioner_6000AI : public ScriptedAI
{
    boss_electrocutioner_6000AI(Creature* pCreature) : ScriptedAI(pCreature) { Reset(); }

    uint32 m_uiShockTimer;
    uint32 m_uiChainBoltTimer;
    uint32 m_uiMegavoltTimer;

    void Reset() override
    {
        m_uiShockTimer = urand(4000, 8000);
        m_uiChainBoltTimer = urand(8000, 13000);
        m_uiMegavoltTimer = urand(12000, 18000);
    }

    void Aggro(Unit* pWho) override { DoScriptText(EMOTE_AGGRO, m_creature); }
    void JustDied(Unit* pKiller) override { DoScriptText(EMOTE_DEATH, m_creature); }

    void UpdateAI(const uint32 uiDiff) override
    {
        if (!m_creature->SelectHostileTarget() || !m_creature->GetVictim())
            return;

        if (m_uiShockTimer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature->GetVictim(), SPELL_SHOCK) == CAST_OK)
                m_uiShockTimer = urand(4000, 8000);
        }
        else
            m_uiShockTimer -= uiDiff;

        if (m_uiChainBoltTimer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature->GetVictim(), SPELL_CHAIN_BOLT) == CAST_OK)
                m_uiChainBoltTimer = urand(8000, 13000);
        }
        else
            m_uiChainBoltTimer -= uiDiff;

        if (m_uiMegavoltTimer < uiDiff)
        {
            if (DoCastSpellIfCan(m_creature, SPELL_MEGAVOLT) == CAST_OK)
                m_uiMegavoltTimer = urand(12000, 18000);
        }
        else
            m_uiMegavoltTimer -= uiDiff;

        DoMeleeAttackIfReady();
    }
};

UnitAI* GetAI_boss_electrocutioner_6000(Creature* pCreature)
{
    return new boss_electrocutioner_6000AI(pCreature);
}

void AddSC_boss_electrocutioner_6000()
{
    Script* pNewScript = new Script;
    pNewScript->Name = "boss_electrocutioner_6000";
    pNewScript->GetAI = &GetAI_boss_electrocutioner_6000;
    pNewScript->RegisterSelf();
}

