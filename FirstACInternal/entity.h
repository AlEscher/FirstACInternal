#pragma once
#include <windows.h>
#include <cstdio>
#include <cstdint>

// Created with ReClass.NET 1.2 by KN4CK3R

struct vec3 { float x, y, z; };

using Vector3 = vec3;

class Player
{
public:
	char pad_0000[4]; //0x0000
	Vector3 headCoord; //0x0004
	char pad_0010[36]; //0x0010
	Vector3 posCoord; //0x0034
	Vector3 viewAngles; //0x0040
	char pad_004C[28]; //0x004C
	int32_t isOnGround; //0x0068
	char pad_006C[4]; //0x006C
	bool bStandingStill; //0x0070
	bool bScoped; //0x0071
	char pad_0072[16]; //0x0072
	int8_t noclip4; //0x0082
	int8_t bInvisible; //0x0083
	char pad_0084[116]; //0x0084
	int32_t health; //0x00F8
	int32_t armour; //0x00FC
	char pad_0100[20]; //0x0100
	int32_t pistolReserveAmmo; //0x0114
	int32_t carbineReserveAmmo; //0x0118
	int32_t shotgunReserveAmmo; //0x011C
	int32_t N0000007F; //0x0120
	int32_t sniperReserveAmmo; //0x0124
	int32_t rifleReserveAmmo; //0x0128
	char pad_012C[16]; //0x012C
	int32_t pistolAmmo; //0x013C
	int32_t carbineAmmo; //0x0140
	int32_t shotgunAmmo; //0x0144
	int32_t smgAmmo; //0x0148
	int32_t sniperAmmo; //0x014C
	int32_t rifleAmmo; //0x0150
	char pad_0154[4]; //0x0154
	int32_t grenadeAmmo; //0x0158
	char pad_015C[8]; //0x015C
	int32_t pistolWaittime; //0x0164
	int32_t carbineWaittime; //0x0168
	int32_t shotgunWaittime; //0x016C
	int32_t smgWaittime; //0x0170
	int32_t sniperWaittime; //0x0174
	int32_t rifleWaittime; //0x0178
	char pad_017C[4]; //0x017C
	int32_t grenadeWaittime; //0x0180
	char pad_0184[8]; //0x0184
	int32_t pistolShots; //0x018C
	int32_t carbineShots; //0x0190
	char pad_0194[12]; //0x0194
	int32_t rifleShots; //0x01A0
	char pad_01A4[4]; //0x01A4
	int32_t grenadesThrown; //0x01A8
	char pad_01AC[80]; //0x01AC
	int32_t numOfKills; //0x01FC
	char pad_0200[36]; //0x0200
	bool bAttacking; //0x0224
	char name[16]; //0x0225
	char pad_0235[247]; //0x0235
	int32_t team; //0x032C
	char pad_0330[8]; //0x0330
	int8_t fly; // 0x338
	char pad_0339; //0x339
	int8_t entTypeSpec5; //0x033A
	char pad_033B[57]; //0x033B
	class Weapon* currentWeaponPtr; //0x0374
	char pad_0378[1400]; //0x0378
}; //Size: 0x08F0

class Weapon
{
public:
	char pad_0000[4]; //0x0000
	int32_t ID; //0x0004
	class Player* owner; //0x0008
	class N00000C01* guninfo; //0x000C
	int32_t* reserveAmmo; //0x0010
	int32_t* ammo; //0x0014
	int32_t* waittime; //0x0018
	int32_t consecutiveShots; //0x001C
	char pad_0020[228]; //0x0020
}; //Size: 0x0104

class N00000C01
{
public:
	char pad_0000[8]; //0x0000
}; //Size: 0x0008