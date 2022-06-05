dll+lib 链接: https://pan.baidu.com/s/13LDRu6mXC6gaADtrGprNVA 提取码: ujm7
编译成功后记得将dll_ffmpeg4（64位的构建套件对应的是dll_ffmpeg4_64）对应目录下的库复制到可执行文件同一目录。

可执行文件同目录的url.txt为视频监控界面对应通道加载的url地址，如果文件名为url1.txt表示不启用。
格式说明：
通道1则为1:rtsp://192.168.1.128:554/1，依次类推，如果有多个重复的则取最后一个。

如果是编译安卓版本，建议不要使用源码目录下的android配置，里面写死了目标版本28，建议删除该文件夹并重新在qtcreator的项目页面创建。

定时存储以及手动存储的文件视频文件在可执行文件下的video1类似目录下，单个文件存储的视频文件在选择的存储文件对应位置，默认d:/1.mp4，选择固定复选框存储的是1个30s的视频，到了30s就不在存储。

海康萤石：https://hls01open.ys7.com/openlive/6e0b2be040a943489ef0b9bb344b96b8.hd.m3u8

带用户名密码验证的视频流地址格式
rstp://admin:1111@192.168.1.14:554/1/1
卡口摄像机：rtsp://admin:12345&192.168.1.64:554/Streaming/Channels/1?transportmode=unicast

在线视频文件：
http://vfx.mtime.cn/Video/2019/02/04/mp4/190204084208765161.mp4
http://vfx.mtime.cn/Video/2019/03/19/mp4/190319212559089721.mp4
http://vfx.mtime.cn/Video/2019/03/17/mp4/190317150237409904.mp4
http://vfx.mtime.cn/Video/2019/03/14/mp4/190314223540373995.mp4

测试数据，64位WIN10+32位qt5.7+32位ffmpeg3+6路1080P主码流+6路子码流
方案：none+none 	CPU：12%  内存：147MB  GPU：0%
方案：dxva2+none 	CPU：3%   内存：360MB  GPU：38%
方案：d3d11va+none 	CPU：2%   内存：277MB  GPU：62%

方案：none+painter 	CPU：30%  内存：147MB  GPU：0%
方案：dxva2+painter 	CPU：30%  内存：360MB  GPU：38%
方案：d3d11va+painter 	CPU：21%  内存：277MB  GPU：62%

方案：none+yuvopengl  	CPU：17%  内存：177MB  GPU：22%
方案：dxva2+yuvopengl	CPU：25%  内存：400MB  GPU：38%
方案：d3d11va+yuvopengl	CPU：18%  内存：330MB  GPU：65%

方案：qsv+nvopengl	CPU：22%  内存：970MB  GPU：40%
方案：dxva2+nvopengl	CPU：20%  内存：380MB  GPU：40%
方案：d3d11va+nvopengl	CPU：15%  内存：320MB  GPU：62%

测试发现，如果采用64位的ffmpeg4，方案d3d11va+nvopengl，CPU占用大概稳定在6%，udp协议比tcp协议占用更低。

支持音视频同步，播放声音，包括本地视频文件和网络流文件的声音，支持mp3 wav wma等常见格式，可提取音频文件封面等信息