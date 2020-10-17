# 前言
![logo](logo.png)<br>
## 欢迎来到Pyr文档
此插件使用了CPython的api来实现py解释<br>
参考
[http://docs.python.org](http://docs.python.org)<br>
本插件使用了嵌入式扩展<br>
在初始化py环境时引入mc模块<br>
所有接口均在mc模块内
# 接口说明
## 监听器
设置监听器,执行函数<br>
执行的函数需要有一个参数<br>
此参数用来传数据,以字典形式传入<br>
可print来查看<br>
相关事件数据和序号参考
[JSR文档](http://game.xiafox.com/jsrdevdoc.htm#reg_t2)<br>
你也可以参考
[帮助](帮助.txt)<br>
返回值决定是否拦截（True继续，False拦截）<br>
* [x] 1-控制台输入指令
* [x] 2-控制台指令输出
* [x] 3-玩家选择表单
* [x] 4-玩家使用物品
* [x] 5-玩家放置方块
* [x] 6-玩家破坏方块
* [x] 7-玩家打开箱子
* [x] 8-玩家打开木桶
* [x] 9-玩家关闭箱子
* [x] 10-玩家关闭木桶
* [x] 11-玩家放入取出物品
* [x] 12-玩家切换纬度
* [x] 13-生物死亡
* [x] 14-生物受伤
* [x] 15-玩家重生
* [x] 16-聊天监听
* [x] 17-玩家输入文本
* [ ] 18-玩家更新命令方块
* [x] 19-玩家输入指令
* [ ] 20-命令方块指令
* [ ] 21-NPC指令
* [x] 22-玩家加载名字
* [x] 23-玩家离开游戏
* [ ] 24-移动监听
* [x] 25-攻击监听
* [ ] 26-爆炸监听
# 使用方法
保证bds根目录有`chakra.dll`,`python38.dll`,`python38.zip`<br>
[下载链接](https://www.python.org/ftp/python/3.8.6/python-3.8.6-embed-amd64.zip)
创建py文件夹<br>
开服即可<br>
# 更多功能
控制台输入`pyreload`可以重载插件