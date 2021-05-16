#pragma once

// 48 8B C4 55 41 56 41 57 48 8D A8 78 F8 FF FF 48 81 EC 70 08 00 00 48 C7 44 24 50 FE FF FF FF 48 89 58 10 48 89 70 18 48 89 78 20 48 8B ?? ?? ?? ?? ?? 48 33 C4 48 89 85 60 07 00 00 48 8B F1 E8 ?? ?? ?? ?? 48 8B F8 F7 86
#define INTERNAL_LEx_UFunctionBind_Pattern  (BYTE*)"\x48\x8B\xC4\x55\x41\x56\x41\x57\x48\x8D\xA8\x78\xF8\xFF\xFF\x48\x81\xEC\x70\x08\x00\x00\x48\xC7\x44\x24\x50\xFE\xFF\xFF\xFF\x48\x89\x58\x10\x48\x89\x70\x18\x48\x89\x78\x20\x48\x8B\x00\x00\x00\x00\x00\x48\x33\xC4\x48\x89\x85\x60\x07\x00\x00\x48\x8B\xF1\xE8\x00\x00\x00\x00\x48\x8B\xF8\xF7\x86"
#define INTERNAL_LEx_UFunctionBind_Mask     (BYTE*)"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx?????xxxxxxxxxxxxxx????xxxxx"
 



#define LE1_ExecutableName L"MassEffect1.exe"
#define LE1_WindowTitle L"Mass Effect"

#define LE1_UFunctionBind_Pattern  INTERNAL_LEx_UFunctionBind_Pattern
#define LE1_UFunctionBind_Mask     INTERNAL_LEx_UFunctionBind_Mask

#define LE1_GetName_Pattern        (BYTE*)"\x48\x8B\xC4\x48\x89\x50\x10\x57\x48\x83\xEC\x30\x48\xC7\x40\xF0\xFE\xFF\xFF\xFF\x48\x89\x58\x08\x48\x89\x68\x18\x48\x89\x70\x20\x48\x8B\xDA\x48\x8B\xF1\x33\xFF\x89\x78\xE8\x48\x89\x3A\x48\x89\x7A\x08\xC7\x40\xE8\x01\x00\x00\x00\x48\x63\x01\x48\x8D\x00\x00\x00\x00\x00\x85\xC0\x74\x23\x48\x8B\xC8\x48\xC1\xF8\x1D\x83\xE0\x07\x81\xE1\xFF\xFF\xFF\x1F\x48\x03\x4C\xC5\x00"
#define LE1_GetName_Mask           (BYTE*)"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx?????xxxxxxxxxxxxxxxxxxxxxxxxx"



#define LE2_ExecutableName L"MassEffect2.exe"
#define LE2_WindowTitle L"Mass Effect 2"

#define LE2_UFunctionBind_Pattern  INTERNAL_LEx_UFunctionBind_Pattern
#define LE2_UFunctionBind_Mask     INTERNAL_LEx_UFunctionBind_Mask

// 48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 57 48 83 EC 20 48 63 01 48 8D ?? ?? ?? ?? ?? 48 8B DA 48 8B F1 85 C0 74 23
#define LE2_NewGetName_Pattern        (BYTE*)"\x48\x89\x5C\x24\x08\x48\x89\x6C\x24\x10\x48\x89\x74\x24\x18\x57\x48\x83\xEC\x20\x48\x63\x01\x48\x8D\x00\x00\x00\x00\x00\x48\x8B\xDA\x48\x8B\xF1\x85\xC0\x74\x23"
#define LE2_NewGetName_Mask           (BYTE*)"xxxxxxxxxxxxxxxxxxxxxxxxx?????xxxxxxxxxx"



#define LE3_ExecutableName L"MassEffect3.exe"
#define LE3_WindowTitle L"Mass Effect 3"

#define LE3_UFunctionBind_Pattern  INTERNAL_LEx_UFunctionBind_Pattern
#define LE3_UFunctionBind_Mask     INTERNAL_LEx_UFunctionBind_Mask

// 48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 57 48 83 EC 20 48 63 01 48 8D ?? ?? ?? ?? ?? 33 DB 48 8B FA 48 8B F1 85 C0 74 17
#define LE3_NewGetName_Pattern        (BYTE*)"\x48\x89\x5C\x24\x08\x48\x89\x6C\x24\x10\x48\x89\x74\x24\x18\x57\x48\x83\xEC\x20\x48\x63\x01\x48\x8D\x00\x00\x00\x00\x00\x33\xDB\x48\x8B\xFA\x48\x8B\xF1\x85\xC0\x74\x17"
#define LE3_NewGetName_Mask           (BYTE*)"xxxxxxxxxxxxxxxxxxxxxxxxx?????xxxxxxxxxxxx"
