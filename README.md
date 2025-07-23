# FE 智能无炮 挂机脚本

## 布阵码

```
LI5HjH7kAjT/1HrXeldAXJjACH/ddrbkRlZXglL4EUWN3kSVgHHUNWZUq1RBrVc=
```

## 依赖环境

```
AvZ2 241030
```
## 使用方法
1.根据[教程](https://gitee.com/vector-wlc/AsmVsZombies)下载并安装好AsmVsZombies；

2.运行脚本前，需将本项目的所有文件置于同一目录下或将.h文件置于 `AsmVeZombies/inc` 目录下；

3.在VSCode中键入Ctrl+Shift+P，在弹出的命令框中键入AvZ，选择Get AvZ Extension，选择qrmd/GetZombieAbscissas，选择2024_01_29，确保`AsmVeZombies/inc` 目录下有`GetZombieAbscissas`文件夹；

4、根据自己的需求在`main.cpp`文件中更改脚本运行模式, 具体详见`main.cpp`文件中的`MessageStr`部分。

## 关于脚本
可以自由使用, 但请注明来源。

稳定性：10000F冲关SL96次，勉强满足AvZ2四周年活动的要求，详细冲关数据在Test文件夹中。个人测试结果仅供参考！

待处理的问题：可能过不了连续极速、快速关，会因为阳光消耗而破阵（<del>最大的问题是阵本身！</del>）

脚本根据出怪类型将关卡分为5种：

1.慢速 节奏: A | A'a | N | I-N'

2.变速 节奏: A | A'a | N | I-N' 默认wave16已经变速，wave17核弹炸两波（这里处理比较草率，待完善）

3.纯白 节奏 A | A'a | I | N | I

4.快速 节奏：A | A'a | I(c) | N 

5.极速 节奏: A | A'a | I(c) | I'(c) w1和w10在岸路抢核坑

其他细节及特殊处理请翻阅 `main.cpp`, `processor.h`
