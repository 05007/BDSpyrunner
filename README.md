![logo](logo.png)<br>
# 欢迎来到Pyr文档
此插件使用了CPython来实现py解释<br>
参考[http://docs.python.org](http://docs.python.org)<br>
本插件使用了嵌入式扩展<br>
在初始化解释器时增加`mc`模块<br>
# 接口说明
## 监听器
设置监听器,执行函数<br>
执行的函数需要有一个参数<br>
此参数用来传数据,以字典形式传入<br>
可print来查看<br>
相关事件数据和参考[JSR文档](http://game.xiafox.com/jsrdevdoc.htm#reg_t2)<br>
也可以参考[帮助](帮助.txt)<br>
返回值决定是否拦截（True继续，False拦截）<br>
* [x] 后台输入
* [x] 后台输出
* [x] 选择表单
* [x] 使用物品
* [x] 放置方块
* [x] 破坏方块
* [x] 打开箱子
* [x] 打开木桶
* [x] 关闭箱子
* [x] 关闭木桶
* [x] 放入取出
* [x] 切换纬度
* [x] 生物死亡
* [x] 生物受伤
* [x] 玩家重生
* [x] 聊天消息
* [x] 输入文本
* [x] 命令方块
* [x] 输入指令
* [x] 命令方块执行
* [ ] NPC指令
* [x] 加载名字
* [x] 离开游戏
* [x] 玩家攻击
* [x] 爆炸监听
# 使用方法
见[帮助](帮助.txt)
# 更多功能
控制台输入`pyreload`可以重载插件