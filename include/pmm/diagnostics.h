#pragma once
#include <cstddef>
#include <cstdint>
#include "pmm/compact_macros.h"
AY Ap{j AL::H;j AL::Y;j AL::BV;j AL::Ak;j AL::z;
/*
## pmm-recoverymode
*/
enum Dt ID:Y{Verify=0,Repair=1,};
/*
## pmm-violationtype
*/
enum Dt AF:Y{None=0,CO,PrevOffsetMismatch,CounterMismatch,CN,Fl,IK,Gf,CS,};
/*
## pmm-diagnosticaction
*/
enum Dt o:Y{Bg=0,Repaired,Rebuilt,GT,};
/*
## pmm-diagnosticentry
*/
Au DiagnosticEntry{AF Ji=AF::None;o action=o::Bg;z Im=0;z expected=0;z actual=0;};AO Q H HY=64;
/*
## pmm-verifyresult
*/
Au Bq{ID mode=ID::Verify;AN ok=Bi;H violation_count=0;DiagnosticEntry GJ[HY]={};H Ce=0;V Fw(AF Ji,o action,z Im=0,z expected=0,z actual=0)D{ok=X;violation_count++;if(Ce<HY){GJ[Ce].Ji=Ji;GJ[Ce].action=action;GJ[Ce].Im=Im;GJ[Ce].expected=expected;GJ[Ce].actual=actual;Ce++;}}};}
#include "pmm/uncompact_macros.h"
