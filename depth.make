cachesim.o: cachesim.c trace.h common.h memory.h abstract_memory.h \
 statistics.h parsecfg.h direct_cache.h random.h full_cache.h
trace.o: trace.c trace.h common.h
common.o: common.c common.h
parsecfg.o: parsecfg.c parsecfg.h common.h
statistics.o: statistics.c statistics.h parsecfg.h
direct_cache.o: direct_cache.c direct_cache.h abstract_memory.h common.h \
 statistics.h parsecfg.h random.h
full_cache.o: full_cache.c full_cache.h abstract_memory.h common.h \
 statistics.h parsecfg.h random.h
memory.o: memory.c memory.h abstract_memory.h common.h statistics.h \
 parsecfg.h
random.o: random.c random.h parsecfg.h common.h
