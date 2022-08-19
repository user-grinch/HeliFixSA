#include <CPools.h>
#include <assembly.hpp>
#include <extensions/ScriptCommands.h>
#include <extensions/scripting/ScriptCommandNames.h>
#include <CHud.h>

#define NEWS_CHOPPER 488
#define COP_CHOPPER 497
#define COP_CHAR 280
#define NEWS_CHAR 23
#define PILOT_CHAR 61

int &numSearchLights = *(int*)0xC1C96C;
using namespace plugin;
CdeclEvent<AddressList<0x53BFE2, H_CALL>, PRIORITY_BEFORE, ArgPickNone, void()> updateHeliEvent;
ThiscallEvent<AddressList<0x6C6F44, H_CALL>, PRIORITY_BEFORE, ArgPickN<CVehicle*, 0>, void(CVehicle*)> heliBlowUpEvent;

class HeliFix
{
public:

	void SetupHeliCrash(CHeli *&pHeli)
	{
		if (pHeli && !(pHeli->m_pDriver && pHeli->m_pDriver->IsAlive()))
		{
			pHeli->m_autoPilot.m_nCarMission = MISSION_HELI_CRASH_LAND;
			pHeli->m_fHealth = 200.0f; // decrease health to ensure explosion on land

			CPed *pDriver = pHeli->m_pDriver;
			if(pDriver)
			{
				Command<Commands::MARK_CHAR_AS_NO_LONGER_NEEDED>(CPools::GetPedRef(pDriver));
			}

			CPed *pPass = pHeli->m_apPassengers[0];
			if (pPass)
			{
				Command<Commands::MARK_CHAR_AS_NO_LONGER_NEEDED>(CPools::GetPedRef(pPass));
			}
			pHeli = nullptr;
		}
	}

	HeliFix()
	{
		// add driver and a passenger in helis
		injector::MakeInline<0x6C6865, 0x6C6870>([](injector::reg_pack& regs)
		{
			CVehicle *pVeh = (CVehicle*)regs.esi;
			int hVeh = CPools::GetVehicleRef(pVeh);

			if (pVeh->m_nModelIndex == COP_CHOPPER)
			{
				int hDriver, hPass;
				Command<Commands::REQUEST_MODEL>(COP_CHAR);
				Command<Commands::LOAD_ALL_MODELS_NOW>();
				Command<Commands::CREATE_CHAR_INSIDE_CAR>(hVeh, PED_TYPE_COP, COP_CHAR, &hDriver);
				pVeh->m_pDriver = CPools::GetPed(hDriver);
				Command<Commands::CREATE_CHAR_AS_PASSENGER>(hVeh, PED_TYPE_COP, COP_CHAR, 0, &hPass);
				pVeh->m_apPassengers[0] = CPools::GetPed(hPass);
				Command<Commands::MARK_MODEL_AS_NO_LONGER_NEEDED>(COP_CHAR);
			}
			else
			{
				int hDriver, hPass;
				Command<Commands::REQUEST_MODEL>(PILOT_CHAR);
				Command<Commands::REQUEST_MODEL>(NEWS_CHAR);
				Command<Commands::LOAD_ALL_MODELS_NOW>();
				Command<Commands::CREATE_CHAR_INSIDE_CAR>(hVeh, PED_TYPE_CIVMALE, PILOT_CHAR, &hDriver);
				pVeh->m_pDriver = CPools::GetPed(hDriver);
				if (pVeh->m_nModelIndex == NEWS_CHOPPER)
				{
					Command<Commands::CREATE_CHAR_AS_PASSENGER>(hVeh, PED_TYPE_CIVMALE, NEWS_CHAR, 0, &hPass);
					pVeh->m_apPassengers[0] = CPools::GetPed(hPass);
				}

				Command<Commands::MARK_MODEL_AS_NO_LONGER_NEEDED>(NEWS_CHAR);
				Command<Commands::MARK_MODEL_AS_NO_LONGER_NEEDED>(PILOT_CHAR);
			}
		});

		// disable search lights during daytime
		injector::MakeInline<0x6C462A, 0x6C4630>([](injector::reg_pack& regs)
		{
			int hour, min;
			Command<Commands::GET_TIME_OF_DAY>(&hour, &min);

			if (hour > 6 && hour < 22)
			{
				numSearchLights = 0;
			}
			else
			{
				numSearchLights = regs.edx;
			}
		}); 

		// Lose heli control on driver death
		updateHeliEvent += [this]()
		{
			SetupHeliCrash(CHeli::pHelis[0]);
			SetupHeliCrash(CHeli::pHelis[1]);
		};

		// Fix infinite blowing up issue with fire_on_driver plugin
		heliBlowUpEvent += [](CVehicle *pHeli)
		{
			static HMODULE installed = GetModuleHandle("fire_on_driver.asi");
			if (pHeli && installed)
			{
				if (pHeli->m_pDriver)
				{
					pHeli->m_pDriver->Remove();
					pHeli->m_pDriver = nullptr;
				}
				if (pHeli->m_apPassengers[0])
				{
					pHeli->m_apPassengers[0]->Remove();
					pHeli->m_apPassengers[0] = nullptr;
				}
			}	
		};
	};
} heliFix;
