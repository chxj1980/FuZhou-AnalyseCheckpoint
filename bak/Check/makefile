objects = XSAnalyseServer.o AnalyseServer.o LibInfo.o FeatureManage.o FeatureSearchThread.o ConfigRead.o ZBase64.o RedisManage.o ZeromqManage.o NodeHandler.o FaceFeatureDB.o
lib = -lpthread -lmysqlclient -I/usr/include/mysql/ -L/usr/lib64/mysql -L/usr/lib64 -lst_feature_comp -lhiredis -lzmq

XSAnalyseServer: $(objects)
	g++ -g -o  XSCheckServer $(objects) $(lib)
XSAnalyseServer.o: XSAnalyseServer.cpp AnalyseServer.h
	g++ -c -g  XSAnalyseServer.cpp 
AnalyseServer.o: AnalyseServer.cpp AnalyseServer.h DataDefine.h LibInfo.h
	g++ -c -g AnalyseServer.cpp
LibInfo.o: LibInfo.cpp LibInfo.h FeatureManage.h DataDefine.h ZBase64.h
	g++ -c -g LibInfo.cpp
FeatureManage.o: FeatureManage.cpp FeatureManage.h FeatureSearchThread.h
	g++ -c -g FeatureManage.cpp
FeatureSearchThread.o: FeatureSearchThread.cpp FeatureSearchThread.h st_feature_comp.h
	g++ -c -g FeatureSearchThread.cpp
NodeHandler.o: NodeHandler.cpp NodeHandler.h
	g++ -c -g NodeHandler.cpp
FaceFeatureDB.o: FaceFeatureDB.cpp FaceFeatureDB.h NodeHandler.h FaceFeatureStruct.h
	g++ -c -g FaceFeatureDB.cpp
RedisManage.o: RedisManage.cpp RedisManage.h
	g++ -c -g RedisManage.cpp
ZeromqManage.o: ZeromqManage.cpp ZeromqManage.h DataDefine.h
	g++ -c -g ZeromqManage.cpp
ConfigRead.o: ConfigRead.cpp ConfigRead.h
	g++ -c -g ConfigRead.cpp
ZBase64.o: ZBase64.cpp ZBase64.h
	g++ -c -g ZBase64.cpp
	
	
clean:
	rm XSCheckServer $(objects)

cpp:
	rm *.cpp *.h rapidjson/*.h rapidjson/error/*.h rapidjson/internal/*.h	rapidjson/msinttypes/*.h STLib/libst_feature_comp.so STLib/feature_util 
