The structure of the simple neural network is as following:

```mermaid
%% graph 是早期语法，
%% flowchart 是新版语法（推荐使用）。
%% 两者几乎功能一样，但 flowchart 支持更多新特性、更好的布局和语义
%% LR 表示图形整体从左到右
%% TD/TB 表示图形整体从上到下
%% 
%% --> 表示箭头相连
%% --- 表示直线相连
%% -.> 表示虚线箭头相连
%% ==> 表示粗箭头相连
%% ~~~ 表示不透明的线相连
%% A & B--> C & D 可以用 & 表示多个节点同时满足一些连接关系
%% |""| 在连线中间显示文字
%% 文字可以放在连线中间，但是文字左边要多点连线符号
%% subgraph title
%%    graph definition
%% end
%% () 圆角矩形
%% ([]) 广圆角矩形
%% (()) 圆
%% [] 矩形
%% [[]] 两边带竖线矩形
%% [()] 圆柱形
%% >] 凹箭头矩形
%% {} 菱形
%% {{}} 凸箭头矩形
%% [/ /] 右平行四边形
%% [\ \] 左平行四边形
%% [/\] 梯形
%% [\/] 倒梯形
%% ((())) 双层圆
%% mermaid 对 latex 支持的目前并不是太好
graph LR
%% 输入层
    X(("x"))
%% 隐藏层 3 个神经元
    H1((h1))
    H2((h2))
    H3((h3))
%% 输出层
    Y((ŷ))
%% 输入到隐藏层连接
    X -- " w11 " ---> H1
    X -->|" w12 "| H2
    X -->|" w13 "| H3
%% 隐藏层到输出层连接
    H1 --->|" w21 "| Y
    H2 --->|" w22 "| Y
    H3 --->|" w23 "| Y
%% 输出层标注
%% ŷ = Σ(w2j * oj) + b2
```

最终的输出层是否需要激活函数取决于问题，

- 预测连续数值时不需要激活函数
- 二分类时需要 sigmoid 激活函数
- 多分类时需要 softmax 激活函数

```mermaid
graph LR
    X(["x"])
    H1(["h1<br>a1 = w11 * x + b1<br>o1 = sigmoid(a1)"])
    H2(["h2<br>a2 = w12 * x + b2<br>o2 = sigmoid(a2)"])
    H3(["h3<br>a3 = w13 * x + b3<br>o3 = sigmoid(a3)"])
    Y(["ŷ = w21*o1 + w22*o2 + w23*o3 + b"])
    X --> H1
    X --> H2
    X --> H3
    H1 --> Y
    H2 --> Y
    H3 --> Y
```

```mermaid
---
config:
  layout: fixed
  theme: default
  look: classic
---
flowchart LR
    subgraph t1["Time step t=1"]
        RNN1["RNN cell"]
        x1["Input x1"]
        h1["Hidden state h1"]
        y1["Output y1"]
    end
    subgraph t2["Time step t=2"]
        RNN2["RNN cell"]
        x2["Input x2"]
        h2["Hidden state h2"]
        y2["Output y2"]
    end
    subgraph t3["Time step t=3"]
        RNN3["RNN cell"]
        x3["Input x3"]
        h3["Hidden state h3"]
        y3["Output ŷ3"]
    end
    x1 --> RNN1
    RNN1 --> h1
    h1 --> y1
    x2 --> RNN2
    RNN2 --> h2
    h2 --> y2
    x3 --> RNN3
    RNN3 --> h3
    h3 --> y3
    h1 -.-> RNN2
    h2 -.-> RNN3
    RNN1:::cell
    RNN2:::cell
    RNN3:::cell
    classDef cell fill: #d0ebff, stroke: #1c7ed6, stroke-width: 2px, color: #000

```

```mermaid
---
config:
  layout: fixed
  theme: default
  look: classic
---
flowchart LR
    subgraph t1["Time step t=1"]
        RNN11["RNN cell"]
        x1["Input x1"]
        h11["Hidden state h11"]
        RNN12["RNN cell"]
        h12["Hidden state h12"]
        y1["Output y1"]
    end
    subgraph t2["Time step t=2"]
        RNN21["RNN cell"]
        RNN22["RNN cell"]
        x2["Input x2"]
        h21["Hidden state h21"]
        h22["Hidden state h22"]
        y2["Output y2"]
    end
    subgraph t3["Time step t=3"]
        RNN31["RNN cell"]
        RNN32["RNN cell"]
        x3["Input x3"]
        h31["Hidden state h31"]
        h32["Hidden state h32"]
        y3["Output ŷ3"]
    end
    x1 --> RNN11
    RNN11 --> h11
    h11 --> RNN12
    RNN12 --> h12
    h12 --> y1
    x2 --> RNN21
    RNN21 --> h21
    h21 --> RNN22
    RNN22 --> h22
    h22 --> y2
    h11 -.-> RNN21
    x3 --> RNN31
    RNN31 --> h31
    h31 --> RNN32
    RNN32 --> h32
    h32 --> y3
    h12 -.-> RNN22
    h21 -.-> RNN31
    h22 -.-> RNN32
    RNN11:::cell
    RNN12:::cell
    RNN21:::cell
    RNN22:::cell
    RNN31:::cell
    RNN32:::cell
    classDef cell fill: #d0ebff, stroke: #1c7ed6, stroke-width: 2px, color: #000

```

```mermaid
graph LR
    X[广告吸引力] --> M[品牌信任]
    M --> Y[购买意愿]
    X --> Y
    Z[个人创新性] --> X
    Z --> M
```