lib = -lpthread -lmysqlclient -I/usr/local/include/mysql -L/usr/local/lib/mysql -L/usr/local/lib/st -lst_feature_comp -lhiredis -lzmq
objects = XSAnalyseServer.o AnalyseServer.o LibInfo.o FeatureManage.o FeatureSearchThread.o ConfigRead.o ZBase64.o RedisManage.o MySQLOperation.o ZeromqManage.o NodeHandler.o FaceFeatureDB.o
target=XSAnalyse_Checkpoint

$(target): $(objects)
	g++ -o $(target) $(objects) $(lib)
XSAnalyseServer.o: XSAnalyseServer.cpp AnalyseServer.h
	g++ -c  XSAnalyseServer.cpp 
AnalyseServer.o: AnalyseServer.cpp AnalyseServer.h DataDefine.h LibInfo.h
	g++ -c AnalyseServer.cpp
LibInfo.o: LibInfo.cpp LibInfo.h FeatureManage.h DataDefine.h ZBase64.h
	g++ -c LibInfo.cpp
FeatureManage.o: FeatureManage.cpp FeatureManage.h FeatureSearchThread.h
	g++ -c FeatureManage.cpp
FeatureSearchThread.o: FeatureSearchThread.cpp FeatureSearchThread.h st_feature_comp.h
	g++ -c FeatureSearchThread.cpp
MySQLOperation.o: MySQLOperation.cpp MySQLOperation.h
	g++ -c MySQLOperation.cpp
NodeHandler.o: NodeHandler.cpp NodeHandler.h
	g++ -c NodeHandler.cpp
FaceFeatureDB.o: FaceFeatureDB.cpp FaceFeatureDB.h NodeHandler.h FaceFeatureStruct.h
	g++ -c FaceFeatureDB.cpp
RedisManage.o: RedisManage.cpp RedisManage.h
	g++ -c RedisManage.cpp
ZeromqManage.o: ZeromqManage.cpp ZeromqManage.h DataDefine.h
	g++ -c ZeromqManage.cpp
ConfigRead.o: ConfigRead.cpp ConfigRead.h
	g++ -c ConfigRead.cpp
ZBase64.o: ZBase64.cpp ZBase64.h
	g++ -c ZBase64.cpp
	
clean:
	rm $(target) $(objects)

cpp:
	rm *.cpp *.h rapidjson/*.h rapidjson/error/*.h rapidjson/internal/*.h	rapidjson/msinttypes/*.h STLib/libst_feature_comp.so STLib/feature_util 
