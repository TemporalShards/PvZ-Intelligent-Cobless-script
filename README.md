# FE 智能无炮 挂机脚本

## 布阵码

```
LI5HjH7kAjT/1HrXeldAXJjACH/ddrbkRlZXglL4EUWN3kSVgHHUNWZUq1RBrVc=
```

## 依赖环境

```
AvZ2 240713
```

## 关于脚本
可以自由使用，但请注明来源

目前比较大的问题：可能过不了连续极速、快速关，会因为阳光消耗而破阵

脚本根据出怪类型将关卡分为5种：

1.慢速 节奏: A | A'a | N | I-N'

2.变速 节奏: A | A'a | N | I-N' 默认wave16已经变速，wave17核弹炸两波（这里处理比较草率，待完善）

3.纯白 节奏 A | A'a | I | N | I

4.快速 节奏：A | A'a | I(c) | N |

5.极速 节奏: A | A'a | I(c) | I'(c) w1和w10在岸路抢核坑

其他细节及特殊处理请参阅 `[FE]Intelligent Cobless.cpp`, `processor.h`
