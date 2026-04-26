#pragma once
#include "pmm/address_traits.h"
#include "pmm/storage_backend.h"
#include <cstddef>
#include <cstdint>
#include <cstring>
#if defined(_WIN32) || defined(_WIN64)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
#include "pmm/compact_macros.h"
AY Ap{
/*
## pmm-mmapstorage
*/
I<F AT=W>Dt EY{DF:j E=AT;EY()D=GL;EY(J EY&)=EO;EY&Af=(J EY&)=EO;EY(EY&&An)D:DU(An.DU),Al(An.Al),Fq(An.Fq)
#if defined(_WIN32) || defined(_WIN64)
,Av(An.Av),BQ(An.BQ)
#else
,_fd(An._fd)
#endif
{An.DU=M;An.Al=0;An.Fq=X;
#if defined(_WIN32) || defined(_WIN64)
An.Av=CG;An.BQ=M;
#else
An._fd=-1;
#endif
}~EY(){close();}AN open(J BG*path,H CD)D{if(Fq)B X;if(path==M||CD==0)B X;CD=((CD+AT::P-1)/AT::P)*AT::P;B open_impl(path,CD);}V close()D{if(!Fq)B;close_impl();DU=M;Al=0;Fq=X;}AN is_open()J D{B Fq;}Y*AR()D{B DU;}J Y*AR()J D{B DU;}H S()J D{B Al;}
/*
### pmm-mmapstorage-expand
*/
AN expand(H Dd)D{if(!Fq||Dd==0)B Fq&&Dd==0;H KR=Al/4+Dd;H BM=Al+KR;BM=((BM+AT::P-1)/AT::P)*AT::P;if(BM<=Al)B X;B expand_impl(BM);}AN HT()J D{B X;}Ew:
#if defined(_WIN32) || defined(_WIN64)
Y*DU=M;H Al=0;AN Fq=X;HANDLE Av=CG;HANDLE BQ=M;AN open_impl(J BG*path,H CD)D{Av=CreateFileA(path,GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,M,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,M);if(Av==CG)B X;LARGE_INTEGER existing_size{};if(!GetFileSizeEx(Av,&existing_size)){FD(Av);Av=CG;B X;}if(G<H>(existing_size.QuadPart)<CD){LARGE_INTEGER HU{};HU.QuadPart=G<LONGLONG>(CD);if(!SetFilePointerEx(Av,HU,M,FILE_BEGIN)||!SetEndOfFile(Av)){FD(Av);Av=CG;B X;}}JF size_hi=G<JF>(CD>>32);JF size_lo=G<JF>(CD&0xFFFFFFFF);BQ=Id(Av,M,PAGE_READWRITE,size_hi,size_lo,M);if(BQ==M){FD(Av);Av=CG;B X;}V*FP=MapViewOfFile(BQ,IM,0,0,CD);if(FP==M){FD(BQ);FD(Av);BQ=M;Av=CG;B X;}DU=G<Y*>(FP);Al=CD;Fq=Bi;B Bi;}V close_impl()D{if(DU!=M){FlushViewOfFile(DU,Al);UnmapViewOfFile(DU);}if(BQ!=M){FD(BQ);BQ=M;}if(Av!=CG){FD(Av);Av=CG;}}AN expand_impl(H BM)D{if(DU!=M){FlushViewOfFile(DU,Al);UnmapViewOfFile(DU);DU=M;}if(BQ!=M){FD(BQ);BQ=M;}LARGE_INTEGER HU{};HU.QuadPart=G<LONGLONG>(BM);if(!SetFilePointerEx(Av,HU,M,FILE_BEGIN)||!SetEndOfFile(Av)){JF hi=G<JF>(Al>>32);JF lo=G<JF>(Al&0xFFFFFFFF);BQ=Id(Av,M,PAGE_READWRITE,hi,lo,M);if(BQ!=M){V*FP=MapViewOfFile(BQ,IM,0,0,Al);if(FP!=M)DU=G<Y*>(FP);}B X;}JF size_hi=G<JF>(BM>>32);JF size_lo=G<JF>(BM&0xFFFFFFFF);BQ=Id(Av,M,PAGE_READWRITE,size_hi,size_lo,M);if(BQ==M)B X;V*FP=MapViewOfFile(BQ,IM,0,0,BM);if(FP==M){FD(BQ);BQ=M;B X;}DU=G<Y*>(FP);Al=BM;B Bi;}
#else
Y*DU=M;H Al=0;AN Fq=X;int _fd=-1;AN open_impl(J BG*path,H CD)D{_fd=::open(path,O_RDWR|O_CREAT,0600);if(_fd<0)B X;Au stat st{};if(::fstat(_fd,&st)!=0){::close(_fd);_fd=-1;B X;}if(G<H>(st.st_size)<CD){if(::ftruncate(_fd,G<off_t>(CD))!=0){::close(_fd);_fd=-1;B X;}}V*addr=::mmap(M,CD,PROT_READ|PROT_WRITE,MAP_SHARED,_fd,0);if(addr==MAP_FAILED){::close(_fd);_fd=-1;B X;}DU=G<Y*>(addr);Al=CD;Fq=Bi;B Bi;}V close_impl()D{if(DU!=M)::munmap(DU,Al);if(_fd>=0){::close(_fd);_fd=-1;}}AN expand_impl(H BM)D{if(DU!=M){::munmap(DU,Al);DU=M;}if(::ftruncate(_fd,G<off_t>(BM))!=0){V*addr=::mmap(M,Al,PROT_READ|PROT_WRITE,MAP_SHARED,_fd,0);if(addr!=MAP_FAILED)DU=G<Y*>(addr);B X;}V*addr=::mmap(M,BM,PROT_READ|PROT_WRITE,MAP_SHARED,_fd,0);if(addr==MAP_FAILED)B X;DU=G<Y*>(addr);Al=BM;B Bi;}
#endif
};AK(Fu<EY<>>,"");}
#include "pmm/uncompact_macros.h"
