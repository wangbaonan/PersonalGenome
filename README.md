# **PGP开发文档** betaV1

该分析平台的主要功能是注册、登录、保存并分析由用户上传待分析的基因组数据，然后由SpringBoot调用服务器上的C++程序来进行计算，该程序会实时输出一个进度文件，后端通过轮询监控该文件随时更新Session，以反馈给前端分析任务的进度情况，然后由前端获取到分析结果并将分析结果（JSON）转换为基因组分析报告，反馈给用户，运行前后以及运行期间会有邮箱提醒用户分析进度。



## 后端技术栈

-   SpringBoot 2.7
-   Redis (Session, 验证码)
-   MybatisPlus
-   Maven
-   MySQL
-   C++ Gamma v1



## 需求分析

### 功能需求

-   用户注册和登录
-   上传基因组数据
-   基因组数据分析
-   分析任务进度监控
-   分析结果反馈

### 技术需求

-   API : RESTful风格
-   SpringBoot的Session, Redis来保存Session
-   使用MybatisPlus进行数据库操作
-   注册和登录需要通过邮箱验证码来验证，Redis



## 设计方案

### 数据库设计

在该项目中，使用MySQL和Redis数据库，并使用MybatisPlus作为数据访问层框架。具体的数据库设计如下：

-   用户表（user）
    
    -   id：主键，自增长
    -   username：用户名
    -   password：密码
    -   email：电子邮件地址
    -   verified：是否验证邮箱
-   数据表（data）
    
    -   id：主键，自增长
    -   user_id：用户ID，外键
    -   file_name：文件名
    -   file_path：文件路径



### API设计

-   用户注册
    
    -   URL：/register
    -   请求方法：POST
    -   参数：
        -   username：用户名
        -   password：密码
        -   email：电子邮件地址
    -   响应：
        -   成功：返回状态码200和用户信息
        -   失败：返回状态码400和错误信息
-   用户登录
    
    -   URL：/login
    -   请求方法：POST
    -   参数：
        -   username：用户名
        -   password：密码
    -   响应：
        -   成功：返回状态码200和用户信息
        -   失败：返回状态码400和错误信息
-   上传基因组数据
    
    -   URL：/data/upload
    -   请求方法：POST
    -   参数：
        -   file：上传的文件
    -   响应：
        -   成功：返回状态码200和文件信息
        -   失败：返回状态码400和错误信息
-   数据分析进度
    
    -   URL：/data/progress
    -   请求方法：GET
    -   响应：
        -   成功：返回状态码200和任务进度信息
        -   失败：返回状态码400和错误信息
-   数据分析结果
    
    -   URL：/data/result
    -   请求方法：GET
    -   响应：
        -   成功：返回状态码200和分析结果信息
        -   失败：返回状态码400和错误信息



### RESTful API
RESTful是一种基于HTTP协议的Web服务设计风格，其主要原则包括：

-   资源标识符（URI）：每个资源都有唯一的URI进行标识，例如/users/1。
-   统一接口：每个资源都采用同一种操作接口，包括GET、POST、PUT、DELETE等。
-   资源的自描述性：每个资源应该包含足够的信息，可以自我描述，例如使用JSON格式表示资源。
-   超媒体作为应用状态引擎（HATEOAS）：每个资源都应该包含可以引导用户进一步操作的链接。

#### 本项目包含以下API：
-   用户注册：POST /api/register，请求体包含邮箱、密码、验证码。
-   获取验证码：GET /api/verificationCode?email={email}。
-   用户登录：POST /api/login，请求体包含邮箱、密码。
-   上传文件：POST /api/upload，请求体包含文件。
-   选择样本和参数进行分析:POST /api/analysis, 请求体包含样本ID、路径、参考基因组版本
-   获取任务进度：GET /api/progress，响应体包含进度信息。
-   获取分析结果：GET /api/report，响应体包含分析结果。



## 功能列表

1. 用户注册
2. 用户登录
3. 上传基因组数据
4. CppPipeline对VCF数据进行分析
5. 查看分析进度
6. 查看分析结果



## API设计

本项目API 为RESTful风格，下面是API的具体设计：



### 用户注册

- URL：/api/register
- 方法：POST
- 请求参数：

| 参数名  | 类型   | 是否必须 | 说明           |
| ------- | ------ | -------- | -------------- |
| email   | string | 是       | 用户邮箱       |
| name    | string | 是       | 用户名         |
| password| string | 是       | 用户密码       |
| code    | string | 是       | 邮箱验证码     |

- 返回值：

  ```json
  { 
      "code": 200, 
      "message": "注册成功" 
  }
  ```



### 用户登录

- URL：/api/login
- 方法：POST
- 请求参数：

| 参数名  | 类型   | 是否必须 | 说明           |
| ------- | ------ | -------- | -------------- |
| email   | string | 是       | 用户邮箱       |
| password| string | 是       | 用户密码       |

- 返回值：

```json
{ 
    "code": 200, 
    "message": "登录成功", 
    "data": { 
        "token": "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiIxMjM0NTY3ODkwIiwibmFtZSI6IkpvaG4gRG9lIiwiaWF0IjoxNTE2MjM5MDIyfQ.SflKxwRJSMeKKF2QT4fwpMeJf36POk6yJV_adQssw5c" 
    } 
}
```



### 上传基因组数据

- URL：/api/upload
- 方法：POST
- 请求参数：

| 参数名 | 类型   | 是否必须 | 说明             |
| ------ | ------ | -------- | ---------------- |
| file   | file   | 是       | 基因组数据文件   |
| token  | string | 是       | 用户登录后的token|

- 返回值：

```json
{
    "code": 200,
    "message": "上传成功",
    "Path": "XXX/XXX.vcf.gz"
}
```



### 选择样本调用后端C++Pipeline进行分析

- URL：/api/analysis
- 方法：POST
- 请求参数：

| 参数名          | 类型   | 是否必须 | 是否可复选 | 说明                                                         |
| --------------- | ------ | -------- | ---------- | ------------------------------------------------------------ |
| token           | string | 是       | 否         | 用户登录后的token                                            |
| sampleID        | string | 是       | 是         | 用户选择的样本ID                                             |
| samplePath      | string | 是       | 是         | 用户选择的样本分析路径                                       |
| assemblyVersion | string | 是       | 否         | 用户选择的样本对应的参考基因组（用户选择多样本并行分析时参考基因组须一致） |
| analysisModules | string | 是       | 是         | AS2/ADMIXTURE/HLA/MT_Y/PRS/SNPedia/SV                        |



### 查看分析进度

- URL：/api/progress
- 方法：GET
- 请求参数：

| 参数名 | 类型   | 是否必须 | 说明              |
| ------ | ------ | -------- | ----------------- |
| token  | string | 是       | 用户登录后的token |



----

# C++ pipeline results (JSON)



## Ancestry (Admixture + NNLS) && Similarity (Euclidean Distance)

**ancestry.json**

> ```json
> [
>     {
>         "Population": "Bouyei",
>         "Region": "Est_Asia",
>         "Language Family": "Tai-Kadai",
>         "Percent": "0.00746263650930532"
>     },
>     {
>         "Population": "Chaoxian",
>         "Region": "Est_Asia",
>         "Language Family": "Koreanic",
>         "Percent": "0.007302369506452941"
>     }
> 
> ]
> ```

`// TODO 添加参考人群的结果，以给出Target的某个成分超越了X%的相似度最高的人群`

**similarity.ED.json**

> ```json
> [
> 
> 	{
>     	"Euclidean Distance": 0.013924734216494051,
>     	"Population": "CHS"
> 	}
> 
> ]
> ```



## Archaic Introgression (ArchaicSeeker V2.0)

**archaic.sum.json**

> ```json
> {
>     "ID": "XR1103",
>     "YRI_Modern_Denisova_Vindija33.19_Altai(cM)": "0.14286%",
>     "YRI_Modern(cM)": "0%",
>     "YRI(cM)": "0.000951625%",
>     "Modern(cM)": "0.162416%",
>     "Denisova_Vindija33.19_Altai(cM)": "0.0112219%",
>     "Denisova(cM)": "0.0634367%",
>     "Vindija33.19_Altai(cM)": "0.145248%",
>     "Vindija33.19(cM)": "1.14774%",
>     "Altai(cM)": "0.174219%"
> }
> ```



**archaic.seg.json**

> ```json
> [
>     {
>         "ID": "XR1103_1",
>         "Contig": "1",
>         "Start(bp)": "2806088",
>         "End(bp)": "2886066",
>         "Start(cM)": "3.68476",
>         "End(cM)": "3.86281",
>         "BestMatchedPop": "Vindija33.19",
>         "BestMatchedTime": "13.4863"
>     },
>     {
>         "ID": "XR1103_1",
>         "Contig": "1",
>         "Start(bp)": "2897968",
>         "End(bp)": "2933853",
>         "Start(cM)": "3.9645",
>         "End(cM)": "4.00066",
>         "BestMatchedPop": "Vindija33.19",
>         "BestMatchedTime": "1e-128"
>     }
> 
> ]
> ```



**david.anno.chartReport.json**

> ```json
> [
>     {
>         "Category": "GOTERM_MF_DIRECT",
>         "Term": "GO:0005132~type I interferon receptor binding",
>         "Count": "14",
>         "%": "1.55555555556",
>         "Pvalue": "3.08466201358e-15",
>         "Genes": "3442, 3441, 3452, 338376, 3440, 3451, 3439, 3449, 3448, 3447, 3446, 3445, 3444, 3443",
>         "List Total": "813",
>         "Pop Hits": "17",
>         "Pop Total": "18908",
>         "Fold Enrichment": "19.1528832935",
>         "Bonferroni": "2.84128276462e-12",
>         "Benjamini": "2.81938108042e-12",
>         "FDR": "2.81321175639e-10"
>     },
>     {
>         "Category": "GOTERM_BP_DIRECT",
>         "Term": "GO:0033141~positive regulation of peptidyl-serine phosphorylation of STAT protein",
>         "Count": "15",
>         "%": "1.66666666667",
>         "Pvalue": "3.53191970562e-15",
>         "Genes": "3442, 3441, 3452, 338376, 3440, 3451, 3439, 3449, 3448, 3447, 3446, 3445, 3444, 3443, 5979",
>         "List Total": "802",
>         "Pop Hits": "21",
>         "Pop Total": "19333",
>         "Fold Enrichment": "17.218560741",
>         "Bonferroni": "1.1603162875e-11",
>         "Benjamini": "1.15317178389e-11",
>         "FDR": "1.1482270963e-09"
>     }
> 
> ]
> ```
>
> 



**david.anno.geneClusterReport.json**

> ```json
> [
>     [
>         {
>             "ENTREZ_GENE_ID": "3449",
>             "Gene Name": "interferon alpha 16(IFNA16)",
>             "Cluster": "1"
>         },
>         {
>             "ENTREZ_GENE_ID": "3448",
>             "Gene Name": "interferon alpha 14(IFNA14)",
>             "Cluster": "1"
>         }
>     ],
>     [
>         {
>             "ENTREZ_GENE_ID": "338785",
>             "Gene Name": "keratin 79(KRT79)",
>             "Cluster": "2"
>         },
>         {
>             "ENTREZ_GENE_ID": "51350",
>             "Gene Name": "keratin 76(KRT76)",
>             "Cluster": "2"
>         }
>     ]
> ]
> ```



**david.anno.termClusteringReport.json**

> ```json
> [
>     [
>         {
>             "Category": "GOTERM_MF_DIRECT",
>             "Term": "GO:0005132~type I interferon receptor binding",
>             "Count": "14",
>             "%": "1.55555555556",
>             "Pvalue": "3.08466201358e-15",
>             "Genes": "3442, 3441, 3452, 338376, 3440, 3451, 3439, 3449, 3448, 3447, 3446, 3445, 3444, 3443",
>             "List Total": "813",
>             "Pop Hits": "17",
>             "Pop Total": "18908",
>             "Fold Enrichment": "19.1528832935",
>             "Bonferroni": "2.84128276462e-12",
>             "Benjamini": "2.81938108042e-12",
>             "FDR": "2.81321175639e-10",
>             "Cluster": "1"
>         },
>         {
>             "Category": "GOTERM_BP_DIRECT",
>             "Term": "GO:0033141~positive regulation of peptidyl-serine phosphorylation of STAT protein",
>             "Count": "15",
>             "%": "1.66666666667",
>             "Pvalue": "3.53191970562e-15",
>             "Genes": "3442, 3441, 3452, 338376, 3440, 3451, 3439, 3449, 3448, 3447, 3446, 3445, 3444, 3443, 5979",
>             "List Total": "802",
>             "Pop Hits": "21",
>             "Pop Total": "19333",
>             "Fold Enrichment": "17.218560741",
>             "Bonferroni": "1.1603162875e-11",
>             "Benjamini": "1.15317178389e-11",
>             "FDR": "1.1482270963e-09",
>             "Cluster": "1"
>         }
>     ],
>     [
>         {
>             "Category": "INTERPRO",
>             "Term": "IPR003054:Type II keratin",
>             "Count": "11",
>             "%": "1.22222222222",
>             "Pvalue": "1.64805577306e-07",
>             "Genes": "3848, 3849, 51350, 3890, 374454, 3891, 3892, 338785, 3887, 3888, 3889",
>             "List Total": "845",
>             "Pop Hits": "28",
>             "Pop Total": "19176",
>             "Fold Enrichment": "8.91530008453",
>             "Bonferroni": "0.000203514192909",
>             "Benjamini": "0.000101767443986",
>             "FDR": "0.01009434161",
>             "Cluster": "2"
>         },
>         {
>             "Category": "GOTERM_MF_DIRECT",
>             "Term": "GO:0030280~structural constituent of epidermis",
>             "Count": "11",
>             "%": "1.22222222222",
>             "Pvalue": "2.44607977419e-06",
>             "Genes": "3848, 3849, 51350, 3890, 374454, 3891, 3892, 338785, 3887, 3888, 3889",
>             "List Total": "813",
>             "Pop Hits": "37",
>             "Pop Total": "18908",
>             "Fold Enrichment": "6.91426481832",
>             "Bonferroni": "0.00223322228828",
>             "Benjamini": "0.00111785845681",
>             "FDR": "0.111541237703",
>             "Cluster": "2"
>         }
>     ]
> ]
> ```



**archaic.seg**

> | ID       | Contig | Start(bp) | End(bp) | Start(cM) | End(cM) | BestMatchedPop | BestMatchedTime |
> | -------- | ------ | --------- | ------- | --------- | ------- | -------------- | --------------- |
> | XR1103_1 | 1      | 2806088   | 2886066 | 3.68476   | 3.86281 | Vindija33.19   | 13.4863         |
> | XR1103_1 | 1      | 2897968   | 2933853 | 3.9645    | 4.00066 | Vindija33.19   | 1e-128          |



**archaicChrPlot.png / archaicChrPlot.svg**

> ![image-20230330194754078](C:\Users\86183\AppData\Roaming\Typora\typora-user-images\image-20230330194754078.png)



## HLA

**HLA.json**

> ```json
> [
>     {
>         "XR1103": {
>             "HLA-DRB1": "DRB1*09:01,DRB1*14:01",
>             "HLA-DQB1": "DQB1*03:03,DQB1*05:03",
>             "HLA-DPA1": "DPA1*01:03,DPA1*02:02",
>             "HLA-DPB1": "DPB1*02:01,DPB1*05:01",
>             "HLA-C": "C*01:02,C*07:02",
>             "HLA-DQA1": "DQA1*01:01,DQA1*03:01",
>             "HLA-B": "B*40:01,B*46:01",
>             "HLA-A": "A*02:07,A*26:01"
>         }
>     }
> ]
> ```

`// TODO 添加特定HLA和疾病的对应关系`

## MT_Y Haplotype group  && Classification && Region

**MT_Y.json**

> ```json
> [
>     {
>         "XR1103": {
>             "Y": "No data of Y chromosome in the input file.",
>             "MT": "M7c1a4a",
>             "Classification": "Han",
>             "Region": "Southeast"
>         }
>     }
> ]
> ```

Tips:女性样本或Y位点过少时没有Y单倍群分型结果



## PCA

**PCA.json**

> ```json
> [
>     {
>         "Population": "XR1103",
>         "Region": "XR1103",
>         "SampleID": "XR1103",
>         "PC1": "0.0198899",
>         "PC2": "0.0011226",
>         "PC3": "0.0143032",
>         "Label": "Target"
>     },
>     {
>         "Population": "UYG",
>         "Region": "Est_Asia",
>         "SampleID": "AAGC021959D",
>         "PC1": "-0.013449",
>         "PC2": "-0.0136736",
>         "PC3": "-0.0105152",
>         "Label": "Panel"
>     }
> ]
> ```



## Province

**province.json**

> ```json
> [
>     {
>         "Best_Matched_Province": "shang_hai"
>     },
>     {
>         "shang_hai": "0.16284251472635503",
>         "qing_hai": "0.16296241650347398",
>         "liao_ning": "0.16305776229472496",
>         "zhe_jiang": "0.16318962558204156",
>         "hei_long_jiang": "0.16326131048365108",
>         "ji_lin": "0.16327414700648551",
>         "hu_bei": "0.16329150315023971",
>         "he_bei": "0.1633071944062646",
>         "jiang_xi": "0.16331167581294667",
>         "shan_dong": "0.16332000647062137",
>         "jiang_su": "0.16333110276091292",
>         "tian_jin": "0.16333448705874087",
>         "he_nan": "0.16338913164201344",
>         "hu_nan": "0.1633933484581989",
>         "nei_meng_gu": "0.1634218181392976",
>         "shan_xi": "0.16344550626637216",
>         "bei_jing": "0.1634947606433373",
>         "fu_jian": "0.1635090494123722",
>         "chong_qing": "0.163543155303316",
>         "tai_wan": "0.163544841238853",
>         "an_hui": "0.16355418193501703",	
>         "xin_jiang": "0.16356236475725058",
>         "si_chuan": "0.16361221092413372",
>         "shaan_xi": "0.16361662447034467",
>         "gan_su": "0.1638681107104214",
>         "guang_dong": "0.16390051419863041",
>         "gui_zhou": "0.16396047468894243",
>         "yun_nan": "0.1642576213279003",
>         "ning_xia": "0.1645752847673101",
>         "guang_xi": "0.16510070940884006"
>     }
> ]
> ```



## PRS

**prs.json**

> ```json
> [
>     {
>         "sampleset": "NGS1926",
>         "IID": "AAGC022151D",
>         "DENOM": "4193908",
>         "PGS000081_hmPOS_GRCh38_SUM": "2.237007",
>         "PGS000086_hmPOS_GRCh38_SUM": "8.625443",
>         "PGS000358_hmPOS_GRCh38_SUM": "-0.094533708",
>         "PGS000359_hmPOS_GRCh38_SUM": "-0.05222227789",
>         "PGS000384_hmPOS_GRCh38_SUM": "-0.10900022950000003",
>         "PGS000463_hmPOS_GRCh38_SUM": "0.0",
>         "PGS000606_hmPOS_GRCh38_SUM": "2.0028070000000002",
>         "PGS000617_hmPOS_GRCh38_SUM": "0.018452062399999993",
>         "PGS000637_hmPOS_GRCh38_SUM": "-0.08826067500000002",
>         "PGS000651_hmPOS_GRCh38_SUM": "0.10128482000000001",
>         "PGS000654_hmPOS_GRCh38_SUM": "2.263906",
>         "PGS000704_hmPOS_GRCh38_SUM": "-0.25887956999999984",
>         "PGS000914_hmPOS_GRCh38_SUM": "-1.4140000000000001",
>         "PGS000962_hmPOS_GRCh38_SUM": "0.9521352390000002",
>         "PGS000991_hmPOS_GRCh38_SUM": "-0.136390426",
>         "PGS001049_hmPOS_GRCh38_SUM": "0.15367291000000008",
>         "PGS001141_hmPOS_GRCh38_SUM": "0.5998408700000002",
>         "PGS001226_hmPOS_GRCh38_SUM": "0.19567592399999997",
>         "PGS001233_hmPOS_GRCh38_SUM": "0.22692228999999894",
>         "PGS001260_hmPOS_GRCh38_SUM": "0.380877631",
>         "PGS001282_hmPOS_GRCh38_SUM": "-0.108203613"
>     }
> ]
> ```



**prsPgsCatalogReport.html**

> ![image-20230330195744176](C:\Users\86183\AppData\Roaming\Typora\typora-user-images\image-20230330195744176.png)



## SNPedia

**snpedia.medicalCondiction.json**

> ```json
> [
>     {
>         "Chr": "chr6",
>         "Position": 159692840,
>         "Rsid": 4880,
>         "Assembly": "GRCh38",
>         "Gene": "SOD2",
>         "Genotype": "AA",
>         "Repute": null,
>         "Magnitude": 1.1,
>         "Summary": "complex! see rs4880",
>         "Reference": "https://www.snpedia.com/index.php/rs4880",
>         "Traits": "Mesothelioma"
>     },
>     {
>         "Chr": "chr9",
>         "Position": 133704962,
>         "Rsid": 140559739,
>         "Assembly": "GRCh38",
>         "Gene": "SARDH",
>         "Genotype": "GG",
>         "Repute": "Good",
>         "Magnitude": 0,
>         "Summary": "common in clinvar",
>         "Reference": "https://www.snpedia.com/index.php/rs140559739",
>         "Traits": "Sarcosinemia"
>     }
> ]
> ```

`// TODO 添加参考人群的Genotype以及frequency以作比较，将R提取的注释用的文件也转为JSON存入MongoDB中，Ref和Target后续都仅需Genotype信息即可`

**snpedia.medicine.json**

> ```json
> [
>     {
>         "Chr": "chr12",
>         "Position": 6845711,
>         "Rsid": 5443,
>         "Assembly": "GRCh38",
>         "Gene": "GNB3",
>         "Genotype": "TT",
>         "Repute": "Bad",
>         "Magnitude": 2.4,
>         "Summary": "viagra is more likely to have an effect. increased metabolic disease risk.",
>         "Reference": "https://www.snpedia.com/index.php/rs5443",
>         "Traits": "Triptans"
>     },
> 	{
>         "Chr": "chr10",
>         "Position": 114045297,
>         "Rsid": 1801253,
>         "Assembly": "GRCh38",
>         "Gene": "ADRB1",
>         "Genotype": "GC",
>         "Repute": null,
>         "Magnitude": null,
>         "Summary": "depends on [[rs1801252]]",
>         "Reference": "https://www.snpedia.com/index.php/rs1801253",
>         "Traits": "Carvedilol"
>     }
> ]
> ```



## SV

**sv.anno.json**

> ```json
> [
>     {
>         "#chrom": "chr1",
>         "start": "8668967",
>         "end": "8669044",
>         "svtype": "DEL",
>         "ID": "306",
>         "svsize": "77",
>         "sv_CDS_overlap": "-",
>         "sv_UTR_overlap": "RERE",
>         "sv_intron_overlap": "-",
>         "pggsv_link": "https://www.biosino.org/pggsv/#/search/detailSV?svId=chr1_8668961_91_DEL&version=GRCh38",
>         "pggsv_sv_ID": "chr1_8668961_91_DEL",
>         "pggsv_sv_region": "8668961-8669051",
>         "dbVar_sv_ID": "-",
>         "dbVar_sv_region": "-",
>         "DGV_sv_ID": "esv2743552",
>         "DGV_sv_region": "8668968-8669042"
>     },
>     {
>         "#chrom": "chr1",
>         "start": "8843980",
>         "end": "8843985",
>         "svtype": "INS",
>         "ID": "307",
>         "svsize": "128",
>         "sv_CDS_overlap": "-",
>         "sv_UTR_overlap": "-",
>         "sv_intron_overlap": "-",
>         "pggsv_link": "https://www.biosino.org/pggsv/#/search/detailSV?svId=chr1_8843980_128_INS&version=GRCh38",
>         "pggsv_sv_ID": "chr1_8843980_128_INS",
>         "pggsv_sv_region": "8843980-8843985",
>         "dbVar_sv_ID": "esv1783066",
>         "dbVar_sv_region": "8844008-8844008",
>         "DGV_sv_ID": "-",
>         "DGV_sv_region": "-"
>     }
> ]
> ```


