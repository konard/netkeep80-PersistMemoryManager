#pragma once
#include "pmm/diagnostics.h"
#include "pmm/types.h"
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#if defined(_WIN32) || defined(_WIN64)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <io.h>
#include <windows.h>
#else
#include <cstdlib>
#include <fcntl.h>
#include <unistd.h>
#endif
#include "pmm/compact_macros.h"
AY Ap{AY K{AO AN atomic_rename(J BG*Hj,J BG*final_path)D{
#if defined(_WIN32) || defined(_WIN64)
B MoveFileExA(Hj,final_path,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0;
#else
B AL::rename(Hj,final_path)==0;
#endif
}AO AN flush_file_to_storage(AL::FILE*file)D{if(file==M)B X;if(AL::fflush(file)!=0)B X;
#if defined(_WIN32) || defined(_WIN64)
int fd=::_fileno(file);B fd>=0&&::_commit(fd)==0;
#else
int fd=::fileno(file);B fd>=0&&::fsync(fd)==0;
#endif
}
#if !defined(_WIN32) && !defined(_WIN64)
AO AL::string parent_directory_path(J BG*path){AL::string DX(path);H slash=DX.find_last_of('/');if(slash==AL::string::npos)B ".";if(slash==0)B "/";B DX.substr(0,slash);}
#endif
AO AN flush_parent_directory(J BG*path){
#if defined(_WIN32) || defined(_WIN64)
(V)path;B Bi;
#else
AL::string BX=parent_directory_path(path);int Iz=O_RDONLY;
#ifdef O_DIRECTORY
Iz|=O_DIRECTORY;
#endif
int fd=::open(BX.GN(),Iz);if(fd<0)B X;AN ok=::fsync(fd)==0;if(::close(fd)!=0)ok=X;B ok;
#endif
}}I<F KA>AO AN save_manager(J BG*Ht){j E=F KA::E;if(Ht==M)B X;AL::vector<Y>HI;{F KA::AP::Ax Eo(KA::CY);if(!KA::Es())B X;J Y*Dm=KA::EG().AR();H total=KA::EG().S();if(Dm==M||total==0)B X;HI.resize(total);AL::Hz(HI.Dm(),Dm,total);}DH*AD=K::Dq<E>(HI.Dm());AD->crc32=K::IH<E>(HI.Dm(),HI.IZ());AL::string Hj=AL::string(Ht)+".tmp";AL::FILE*f=AL::fopen(Hj.GN(),"wb");if(f==M)B X;H written=AL::fwrite(HI.Dm(),1,HI.IZ(),f);AN ok=written==HI.IZ();if(ok)ok=K::flush_file_to_storage(f);if(AL::fclose(f)!=0)ok=X;if(!ok){AL::JQ(Hj.GN());B X;}if(!K::atomic_rename(Hj.GN(),Ht)){AL::JQ(Hj.GN());B X;}if(!K::flush_parent_directory(Ht))B X;B Bi;}I<F KA>AO AN load_manager_from_file(J BG*Ht,Bq&AM){j E=F KA::E;if(Ht==M)B X;Y*buf=KA::EG().AR();H IZ=KA::EG().S();if(buf==M||IZ<K::Fm)B X;AL::FILE*f=AL::fopen(Ht,"rb");if(f==M)B X;if(AL::fseek(f,0,SEEK_END)!=0){AL::fclose(f);B X;}long file_size_long=AL::ftell(f);if(file_size_long<=0){AL::fclose(f);B X;}AL::rewind(f);H JB=G<H>(file_size_long);if(JB>IZ){AL::fclose(f);B X;}H read_bytes=AL::fread(buf,1,JB,f);AL::fclose(f);if(read_bytes!=JB)B X;Q H kHdrOffset=K::Bf<E>;if(JB>=kHdrOffset+AG(K::q<E>)){DH*AD=K::Dq<E>(buf);Ak stored_crc=AD->crc32;Ak computed_crc=K::IH<E>(buf,JB);if(stored_crc!=computed_crc){KA::set_last_error(AE::CrcMismatch);KA::AS::Cg(AE::CrcMismatch);B X;}}B KA::load(AM);}}
#include "pmm/uncompact_macros.h"
