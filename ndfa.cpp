/****************************************************
 * @Copyright © 2021-2023 Lyuyk. All rights reserved.
 *
 * @FileName: ndfa.cpp
 * @Brief: NFA、DFA类源文件
 * @Module Function:
 *
 * @Current Version: 1.3
 * @Author: Lyuyk
 * @Modifier: Lyuyk
 * @Finished Date: 2023/4/21
 *
 * @Version History: 1.1
 *                   1.2 增添了部分注释，提高了部分代码的可读性
 *                   1.3 current version
 *
 ****************************************************/
#include "ndfa.h"
#include <QDebug>

NDFA::NDFA()
{
    init();
}

/**
 * @brief NDFA::init
 * 类初始化函数
 */
void NDFA::init()
{
    //初始化FA状态计数
    m_NFAStateNum=0;
    m_DFAStateNum=0;
    m_mDFAStateNum=0;
    m_opCharSet.clear();
    m_DFAEndStateSet.clear();
    m_keyWordSet.clear();
    m_reg_keyword_str.clear();
    m_lexerCodeStr.clear();
    m_dividedSet->clear();

    //FA图初始化
    m_NFAG.startNode=NULL;
    m_NFAG.endNode=NULL;
    m_mDFAG.startState=-1;
    m_mDFAG.endStateSet.clear();

    //NFA节点数组初始化
    for(int i=0;i<ARR_MAX_SIZE;i++)
    {
        m_NFAStateArr[i].init();
        m_NFAStateArr[i].stateNum=i;
    }

    //DFA节点数组初始化
    for(int i=0;i<ARR_MAX_SIZE;i++)
    {
        m_DFAStateArr[i].init();
        m_DFAStateArr[i].stateNum=i;
    }

    //mDFA节点数组初始化优化
    for(int i=0;i<ARR_MAX_SIZE;i++)
    {
        m_mDFANodeArr[i].init();
    }
}

void NDFA::printNFA(QTableWidget *table)
{
    int epsColN=m_opCharSet.size()+1;//最后一列 epsilon 列号

    //初始化表头
    QStringList headerStrList=m_opCharSet.values();
    headerStrList.push_front("状态号");
    headerStrList.push_back("epsilon");
    headerStrList.push_back("初/终态");

    table->setRowCount(m_NFAStateNum);
    table->setColumnCount(m_opCharSet.size()+3);
    //设置表头 行
    table->setHorizontalHeaderLabels(headerStrList);
    //竖轴隐藏
    table->verticalHeader()->setHidden(true);
    table->setAlternatingRowColors(true);//隔行变色
    table->setPalette(QPalette(qRgb(240,240,240)));
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);//自动调整列宽


    for(int state=0;state<m_NFAStateNum;state++)
    {
        table->setItem(state,0,
                       new QTableWidgetItem(QString::number(state)));
        table->item(state,0)->setTextAlignment(Qt::AlignCenter);//居中

        int k=1;
        int colN=1;

        for(;k<headerStrList.size()-2;k++)
        {
            if(m_NFAStateArr[state].value==headerStrList[k])
            {
                table->setItem(state,colN,
                               new QTableWidgetItem(
                                   QString::number(m_NFAStateArr[state].toState)));
                table->item(state,colN)->setTextAlignment(Qt::AlignCenter);//居中
            }
            colN++;
        }

        QString epsStr="";
        //qDebug()<<NFAStateArr[state].epsToSet;
        for(const auto &e_state: m_NFAStateArr[state].epsToSet)
        {
            epsStr+=QString::number(e_state)+",";
        }
        epsStr.chop(1);
        table->setItem(state,epsColN,new QTableWidgetItem(epsStr));
        table->item(state,epsColN)->setTextAlignment(Qt::AlignCenter);//居中

        if(m_NFAStateArr[state].stateNum==m_NFAG.startNode->stateNum)
        {//若为初态
            table->setItem(state,epsColN+1,new QTableWidgetItem("初态"));
            table->item(state,epsColN+1)->setTextAlignment(Qt::AlignCenter);//居中
        }
        if(m_NFAStateArr[state].stateNum==m_NFAG.endNode->stateNum)
        {//若为终态
            table->setItem(state,epsColN+1,new QTableWidgetItem("终态"));
            table->item(state,epsColN+1)->setTextAlignment(Qt::AlignCenter);//居中
        }
    }
}

void NDFA::printDFA(QTableWidget *table)
{
    //初始化
    table->clear();table->setRowCount(0);table->setColumnCount(0);

    table->setRowCount(m_DFAStateNum);//表格行数
    table->setColumnCount(m_opCharSet.count()+3);//表格列数（状态号，包含NFA状态，操作符。。。，初/终态）
    table->verticalHeader()->setHidden(true);//隐藏竖轴序号列
    table->setAlternatingRowColors(true);//隔行变色
    table->setPalette(QPalette(qRgb(240,240,240)));


    //表头初始化
    QStringList headerStrList=m_opCharSet.values();
    headerStrList.push_front("包含的NFA状态");
    headerStrList.push_front("状态号");
    headerStrList.push_back("初/终态");
    //水平表头
    table->setHorizontalHeaderLabels(headerStrList);

    //遍历所有DFA节点
    for(int state=0;state<m_DFAStateNum;state++)
    {
        //设置状态号
        table->setItem(state,0,
                       new QTableWidgetItem(QString::number(m_DFAStateArr[state].stateNum)));
        table->item(state,0)->setTextAlignment(Qt::AlignCenter);//居中
        //NFA状态集
        QString NFASetStr="{ ";
        for(const auto &n_state:m_DFAStateArr[state].NFANodeSet)
        {
            NFASetStr+=QString::number(n_state)+",";
        }
        NFASetStr.replace(NFASetStr.size()-1,1," }");
        table->setItem(state,1,new QTableWidgetItem(NFASetStr));

        int k=2;//从第一个操作符开始
        int colN=2;//第三列开始显示
        for(;k<headerStrList.count()-1;k++)
        {
            QString curOpChar=headerStrList[k];//当前操作符

            if(m_DFAStateArr[state].DFAEdgeMap.contains(curOpChar))//若可达
            {
                //可以去到的状态号
                int toState=m_DFAStateArr[state].DFAEdgeMap[curOpChar];

//                QString n_NFASetStr=QString::number(toState)+" { ";
//                for(const auto &n_state:m_DFAStateArr[toState].NFANodeSet)//可去到的状态集
//                {
//                    n_NFASetStr+=QString::number(n_state)+",";
//                }
//                n_NFASetStr.replace(n_NFASetStr.size()-1,1," }");
                //状态多起来显示集合很麻烦
                table->setItem(state,colN,new QTableWidgetItem(QString::number(toState)));
                table->item(state,colN)->setTextAlignment(Qt::AlignCenter);//居中
            }
            colN++;
        }

        if(m_DFAEndStateSet.contains(state))
        {//若为终态
            table->setItem(state,colN,new QTableWidgetItem("终态"));
            table->item(state,colN)->setTextAlignment(Qt::AlignCenter);//居中
        }
        else if(m_DFAStateArr[state].NFANodeSet.contains(m_NFAG.startNode->stateNum))
        {
            table->setItem(state,colN,new QTableWidgetItem("初态"));
            table->item(state,colN)->setTextAlignment(Qt::AlignCenter);//居中
        }
    }
    table->resizeRowsToContents();//自适应行高
}

void NDFA::printMDFA(QTableWidget *table)
{
    //初始化
    table->clear();table->setRowCount(0);table->setColumnCount(0);

    table->setRowCount(m_mDFAStateNum);//表格行数
    table->setColumnCount(m_opCharSet.count()+2);//表格列数
    table->verticalHeader()->setHidden(true);//隐藏竖轴序号列
    table->setAlternatingRowColors(true);//隔行变色
    table->setPalette(QPalette(qRgb(240,240,240)));
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);//自动调整列宽

    //表头初始化
    QStringList headerStrList=m_opCharSet.values();
    headerStrList.push_front("状态号");
    headerStrList.push_back("初/终态");
    //设置水平表头
    table->setHorizontalHeaderLabels(headerStrList);

    for(int stateId=0;stateId<m_mDFAStateNum;stateId++)
    {
        int k=1;//第一个状态开始
        int colN=1;

        table->setItem(stateId, 0,
                       new QTableWidgetItem(QString::number(stateId)));//列 状态号
        table->item(stateId,0)->setTextAlignment(Qt::AlignCenter);

        //遍历所有操作符
        for( ;k<headerStrList.count()-1;k++)
        {
            QString curOpChar=headerStrList[k];//当前操作符
            if(m_mDFANodeArr[stateId].mDFAEdgesMap.contains(curOpChar))
            {
                //若通过当前操作符可达
                int toIdx=m_mDFANodeArr[stateId].mDFAEdgesMap[curOpChar];

                table->setItem(stateId,colN,new QTableWidgetItem(QString::number(toIdx)));
                table->item(stateId,colN)->setTextAlignment(Qt::AlignCenter);//居中
            }
            colN++;
        }

        if(m_mDFAG.endStateSet.contains(stateId))
        {
            //若为初态
            table->setItem(stateId,colN,
                           new QTableWidgetItem("终态"));
            table->item(stateId,colN)->setTextAlignment(Qt::AlignCenter);//居中
        }
        else if(stateId==m_mDFAG.startState)
        {
            //若为终态
            table->setItem(stateId,colN,
                           new QTableWidgetItem("初态"));
            table->item(stateId,colN)->setTextAlignment(Qt::AlignCenter);//居中
        }
    }
}

void NDFA::printLexer(QPlainTextEdit *widget)
{
    widget->clear();
    widget->setPlainText(m_lexerCodeStr);
}

/**
 * @brief NDFA::strToNfa
 * 将正则表达式转换为NFA
 * @param s
 * @return
 */
NDFA::NFAGraph NDFA::strToNfa(QString s)
{
    opPriorityMapInit();//运算符优先级初始化

    QStack<NFAGraph> NFAStack;//存NFA子图的栈
    QStack<QChar> opStack;//符号栈

    for(int i=0;i<s.size();i++)
    {
        switch(s[i].unicode())
        {
        case '|':
        case '&':
        {
            pushOpStackProcess(s[i],opStack,NFAStack);
            break;
        }
        case '*':
        case '+':
        case '?':
        {
            pushOpStackProcess(s[i],opStack,NFAStack);
            insConnOp(s,i,opStack,NFAStack);
            break;
        }
        case '(': opStack.push('(');break;
        case ')':
        {
            while(!opStack.empty())
            {
                if(opStack.top() != '(')
                {
                    opProcess(opStack.top(),NFAStack);
                    opStack.pop();
                }
                else break;
            }
            opStack.pop();
            insConnOp(s,i,opStack,NFAStack);
            break;
        }
        default:
        {
            //查看是否为转义字符
            QString tmpStr;
            if(s[i]=='\\')
            {
                while(s[++i]!='\\')
                {
                    if(s[i]=='`')i++;//转义的转义字符，因MiniC中注释符号有反斜杠'\'，用于区分
                    tmpStr+=s[i];
                }
                m_opCharSet.insert(tmpStr);//顺便加入操作符集合
                qDebug()<<tmpStr;
            }
            else {
                tmpStr=s[i];

                if(!m_opSet.contains(s[i]))
                {
                    m_opCharSet.insert(QString(s[i]));//若是转义字符顺便加入操作符集合
                    qDebug()<<tmpStr;
                }
            }

            NFAGraph n=createNFA(m_NFAStateNum);
            m_NFAStateNum+=2;
            //生成NFA子图，加非eps边
            add(n.startNode,n.endNode,tmpStr);

            NFAStack.push(n);
            insConnOp(s,i,opStack,NFAStack);
        }
        }
    }
    //读完字符串，运算符站还有元素则将其全部出栈
    while(!opStack.empty())
        opProcess(opStack.pop(),NFAStack);

    return NFAStack.top();
}

/**
 * @brief NDFA::opPriorityMapInit
 * 初始化操作符优先级，数值越高优先级越大
 */
void NDFA::opPriorityMapInit()
{
    opPriorityMap['(']=0;
    opPriorityMap['|']=1;
    opPriorityMap['&']=2;
    opPriorityMap['*']=3;
    opPriorityMap['+']=3;
    opPriorityMap['?']=3;
}

void NDFA::insConnOp(QString str, int curState, QStack<QChar> &opStack, QStack<NFAGraph> &NFAStack)
{
    //判断是否两元素间加入连接符号，即栈中是否需要加入连接符号
    if(curState+1<str.size() && (!m_opSet.contains(str[curState+1]) || str[curState+1]=='('))
    {
        pushOpStackProcess('&',opStack,NFAStack);
    }
}

void NDFA::pushOpStackProcess(QChar opCh, QStack<QChar> &opStack, QStack<NFAGraph> &NFAStack)
{
    while(!opStack.empty())
    {
        //若栈顶运算符优先级大于当前将进栈元素，则将栈顶元素出栈并处理，再继续循环判断
        if(opStack.top()!='(' && opPriorityMap[opStack.top()]>=opPriorityMap[opCh])
        {
            opProcess(opStack.top(),NFAStack);
            opStack.pop();
        }
        else break;
    }
    opStack.push(opCh);
}

/**
 * @brief NDFA::opCharProcess
 * @param opChar
 * @param NFAStack
 * 根据运算符转换NFA处理子函数
 */
void NDFA::opProcess(QChar opChar, QStack<NFAGraph> &NFAStack)
{
    switch(opChar.unicode())
    {
    case '|':
    {
        //或运算处理
        NFAGraph n1=NFAStack.pop();//先出n1
        NFAGraph n2=NFAStack.pop();//后出n2
        NFAGraph n=createNFA(m_NFAStateNum);
        m_NFAStateNum+=2;

        add(n.startNode,n2.startNode);
        add(n.startNode,n1.startNode);
        add(n2.endNode,n.endNode);
        add(n1.endNode,n.endNode);
        NFAStack.push(n);

        break;
    }
    case '&':
    {
        //与运算处理
        NFAGraph n1=NFAStack.pop();
        NFAGraph n2=NFAStack.pop();

        add(n2.endNode,n1.startNode);

        NFAGraph n;
        n.startNode=n2.startNode;
        n.endNode=n1.endNode;

        NFAStack.push(n);
        break;
    }
    case '*':
    {
        //闭包运算处理
        NFAGraph n1=NFAStack.pop();
        NFAGraph n=createNFA(m_NFAStateNum);
        m_NFAStateNum+=2;

        add(n.startNode,n.endNode);
        add(n.startNode,n1.startNode);
        add(n1.endNode,n1.startNode);
        add(n1.endNode,n.endNode);

        NFAStack.push(n);
        break;
    }
    case '+':
    {
        //正闭包运算处理
        NFANode *newEndNode=&m_NFAStateArr[m_NFAStateNum];
        m_NFAStateNum++;

        NFAGraph n1=NFAStack.pop();
        add(n1.endNode,newEndNode);
        add(n1.endNode,n1.startNode);

        NFAGraph n;
        n.startNode=n1.startNode;
        n.endNode=newEndNode;

        NFAStack.push(n);
        break;
    }
    case '?':
    {
        NFANode *newStartNode=&m_NFAStateArr[m_NFAStateNum];
        m_NFAStateNum++;

        NFAGraph n1=NFAStack.pop();
        add(newStartNode,n1.startNode);
        add(newStartNode,n1.endNode);

        NFAGraph n;
        n.startNode=newStartNode;
        n.endNode=n1.endNode;

        NFAStack.push(n);
        break;
    }
    }
}

/**
 * @brief NDFA::get_e_closure
 * @param tmpSet
 * 求epsilon闭包
 */
void NDFA::get_e_closure(QSet<int> &tmpSet)
{
    QQueue<int> q;
    for(const auto &value: tmpSet)
        q.push_back(value);

    while(!q.empty())
    {
        int tmpFront=q.front();
        q.pop_front();

        QSet<int> eTmpSet=m_NFAStateArr[tmpFront].epsToSet;
        //将通过epsilon到达的节点序号放入集合中
        for(const auto &value: eTmpSet)
        {
            if(!tmpSet.contains(value))
            {
                tmpSet.insert(value);
                q.push_back(value);
            }
        }
    }
}

/**
 * @brief NDFA::getStateId
 * @param set
 * @param cur
 * @return i
 * 查询当前DFA节点号cur属于mDFA节点的状态集号
 */
int NDFA::getStateId(QSet<int> set[], int cur)
{
    for(int i=0;i<m_mDFAStateNum;i++)
    {
        if(set[i].contains(cur))
            return i;
    }
}

/**
 * @brief NDFA::genLexCase
 * @param tmpList
 * @param codeStr
 * @param idx
 * @param flag
 * @return
 *生成Lexer代码的核心子函数
 */
bool NDFA::genLexCase(QList<QString> tmpList, QString &codeStr, int idx, bool flag)
{
    bool rFlag=false;
    for(int i=0;i<tmpList.size();i++)
    {
        QString tmpKey=tmpList[i];
        //字母情况
        if(tmpKey=="letter")
        {
            for(int j=0;j<26;j++)
            {
                codeStr+="\t\t\tcase \'"+QString(char('a'+j))+"\':\n";
                codeStr+="\t\t\tcase \'"+QString(char('A'+j))+"\':\n";
            }
            codeStr.chop(1);//去掉末尾字符
            if(flag)codeStr+="isIdentifier = true; ";
        }
        else if(tmpKey=="digit")
        {
            //数字情况
            for(int j=0;j<10;j++)
            {
                codeStr+="\t\t\tcase \'"+QString::number(j)+"\':\n";
            }
            codeStr.chop(1);
            if(flag)codeStr+="isDigit = true; ";
        }
        else if(tmpKey=="~")
        {
            rFlag=true;
            continue;
        }
        else codeStr+="\t\t\tcase \'"+tmpKey+"\':";

        if(flag)codeStr+="state = "+QString::number(m_mDFANodeArr[idx].mDFAEdgesMap[tmpKey])+"; ";
        codeStr+="break;\n";
    }
    return rFlag;
}

/**
 * @brief NDFA::createNFA
 * @param stateN
 * @return n
 * 建立一个初始化的NFA子图，包含开始和结尾两NFA节点
 */
NDFA::NFAGraph NDFA::createNFA(int stateN)
{
    NFAGraph n;

    n.startNode=&m_NFAStateArr[stateN];
    n.endNode=&m_NFAStateArr[stateN+1];

    return n;
}

/**
 * @brief NDFA::add
 * @param n1
 * @param n2
 * @param ch
 * n1--ch-->n2
 * NFA节点n1与n2间添加一条非epsilon边，操作符为ch
 */
void NDFA::add(NDFA::NFANode *n1, NDFA::NFANode *n2, QString ch)
{
    n1->value = ch;
    n1->toState = n2->stateNum;
}

/**
 * @brief NDFA::add
 * @param n1
 * @param n2
 * n1--eps->n2
 * NFA节点n1与n2间添加一条epsilon边，信息记录于n1节点中
 */
void NDFA::add(NFANode *n1, NFANode *n2)
{
    n1->epsToSet.insert(n2->stateNum);
    //qDebug()<<"addEps:"<<n2->stateNum;
}

/**
 * @brief NDFA::reg2NFA
 * @param regStr
 * 将正则表达式转换为NFA的主函数
 */
void NDFA::reg2NFA(QString regStr)
{
    m_NFAG=strToNfa(regStr);//调用转换函数
}

/**
 * @brief NDFA::NFA2DFA
 * 将NFA转换为DFA的主函数
 */
void NDFA::NFA2DFA()
{    
    QSet<int> tmpSet;
    tmpSet.insert(m_NFAG.startNode->stateNum);//将NFA初态节点放入集合
    get_e_closure(tmpSet);//求NFA初态节点的epsilon闭包得到DFA初态
    m_DFAStateArr[m_DFAStateNum].NFANodeSet=tmpSet;//从初态开始
    if(tmpSet.contains(m_NFAG.endNode->stateNum))
        m_DFAEndStateSet.insert(m_DFAStateNum);
    m_DFAStateNum++;

    QSet<QSet<int>> DFAStatesSet;//存储DFA节点包含的序号
    DFAStatesSet.insert(tmpSet);//将出台包含的序号集放入集合
    QQueue<DFANode> q;
    q.push_back(m_DFAStateArr[0]);//初态入队

    while(!q.empty())
    {
        DFANode *t_DFANode=&q.front();//取出队列中一个DFA节点
        q.pop_front();

        tmpSet=t_DFANode->NFANodeSet;//取出该DFA节点所包含的序号集
        int t_curState=t_DFANode->stateNum;//记录当前DFA节点序号

        //遍历操作符集
        for(const auto &ch: m_opCharSet)
        {
            QSet<int> chToSet;//节点的（出边为ch）的集合
            for(const auto &t_state: tmpSet)//遍历当前序号集合中的所有序号
            {
                //找出所有这样的序号
                if(m_NFAStateArr[t_state].value==ch)
                    chToSet.insert(m_NFAStateArr[t_state].toState);
            }

            if(chToSet.empty())//若找不到这样的节点，则说明该边需要被丢弃
                continue;
            get_e_closure(chToSet);//求上得的序号集合的epsilon闭包

            if(!DFAStatesSet.contains(chToSet))
            {
                //若该DFA状态节点不存在
                //新建DFA节点
                m_DFAStateArr[m_DFAStateNum].NFANodeSet=chToSet;//chToSet--ch-->xxx
                //若包含NFA终态
                if(chToSet.contains(m_NFAG.endNode->stateNum))
                    m_DFAEndStateSet.insert(m_DFAStateNum);
                //更新原节点的信息
                m_DFAStateArr[t_curState].DFAEdgeMap[ch]=m_DFAStateNum;//当前DFA节点能通过ch去到的新DFA状态
                DFAStatesSet.insert(chToSet);
                q.push_back(m_DFAStateArr[m_DFAStateNum++]);//节点入队后自增，DFA节点新增，寻找更多的
            }
            else
            {   //若该DFA状态节点存在
                for(int i=0;i<m_DFAStateNum;i++)
                {
                    if(m_DFAStateArr[i].NFANodeSet==chToSet)
                    {
                        //更新当前DFA节点状态
                        m_DFAStateArr[t_curState].DFAEdgeMap[ch]=i;//当前DFA节点能通过ch边去到的DFA状态发生变更
                        break;
                    }
                }
            }
        }
    }

}

/**
 * @brief NDFA::DFA2mDFA
 * DFA的最小化
 */
void NDFA::DFA2mDFA()
{
    m_mDFAStateNum=1;//未划分，状态数量为1
    for(int i=0;i<m_DFAStateNum;i++)//遍历DFA状态集合
    {   //若DFA状态非终态
        if(!m_DFAStateArr[i].NFANodeSet.contains(m_NFAG.endNode->stateNum))
        {   //暂时都划分到非终态集合
            m_dividedSet[1].insert(m_DFAStateArr[i].stateNum);//非终态集合
            m_mDFAStateNum=2;//设为2，终态与非终态

        }
        else m_dividedSet[0].insert(m_DFAStateArr[i].stateNum);//否则加入终态划分
    }

    bool divFlag=true;//表示是否有新状态划分出来，有则真，无则假
    while(divFlag)
    {
        //遍历mDFA状态
        divFlag=false;
        for(int i=0;i<m_mDFAStateNum;i++)
        {
            //遍历操作符集
            for(const auto &opChar: m_opCharSet)
            {
                int t_divCount = 0;//暂存划分状态计数，从0开始
                stateSet t_stateSet[ARR_TEMP_SIZE];//分出的状态

                for(const auto &t_state: m_dividedSet[i])//遍历当前划分状态集中的所有节点
                {
                    //若当前DFA节点能通过opChar边到达另外一个节点
                    if(m_DFAStateArr[t_state].DFAEdgeMap.contains(opChar))
                    {
                        //通过opChar到达的节点所属状态划分集合 号
                        int toIdx=getStateId(m_dividedSet, m_DFAStateArr[t_state].DFAEdgeMap[opChar]);
                        bool haveSameDiv=false;
                        for(int j=0;j<t_divCount;j++)
                        {
                            //若暂时分出来的状态号有相同的
                            if(t_stateSet[j].stateSetId==toIdx)
                            {
                                haveSameDiv=true;//状态划分未变更，仅划分内状态变动
                                t_stateSet[j].DFAStateSet.insert(t_state);//加入该状态入暂存划分状态中
                                break;
                            }
                        }
                        if(!haveSameDiv)
                        {
                            //若状态不在相同的划分，即出现了新的划分
                            t_stateSet[t_divCount].stateSetId=toIdx;
                            t_stateSet[t_divCount].DFAStateSet.insert(t_state);
                            t_divCount++;//暂存划分计数增加
                        }
                    }
                    else
                    {
                        //若当前DFA节点不能通过opChar边到达另外一个节点
                        bool haveSame=false;
                        for(int j=0;j<t_divCount;j++)//遍历所有暂存划分
                        {
                            if(t_stateSet[j].stateSetId==-1)
                            {
                                haveSame=true;
                                t_stateSet[j].DFAStateSet.insert(t_state);
                                break;
                            }
                        }
                        if(!haveSame)
                        {
                            //若状态不在相同的划分，即出现了新的划分
                            t_stateSet[t_divCount].stateSetId=-1;//不可达的标记
                            t_stateSet[t_divCount].DFAStateSet.insert(t_state);
                            t_divCount++;//分出来的新状态增加
                        }
                    }
                }

                if(t_divCount>1)//大于1即产生了新的划分
                {
                    divFlag=true;
                    for(int j=1;j<t_divCount;j++)//遍历暂存划分集合
                    {
                        for(const auto &state: t_stateSet[j].DFAStateSet)
                        {
                            //将被分出去的DFA节点序号从原来状态中去除
                            m_dividedSet[i].remove(state);
                            //加入新状态
                            m_dividedSet[m_mDFAStateNum].insert(state);
                        }
                        m_mDFAStateNum++;
                    }
                }
            }
        }
    }

    //遍历所有mDFA状态
    for(int i=0;i<m_mDFAStateNum;i++)
    {
        m_mDFANodeArr[i].DFAStatesSet=m_dividedSet[i];//保存每一个暂存划分到mDFA中
        for(const auto &state: m_dividedSet[i])//遍历每一个mDFA划分子集
        {
            //若为初态
            if(state==0)
            {
                m_mDFAG.startState=i;
            }
            //若为终态
            if(m_DFAEndStateSet.contains(state))
            {
                m_mDFAG.endStateSet.insert(i);//加入终态集
            }

            //遍历所有操作符
            for(const auto &opChar: m_opCharSet)
            {
                //若当前状态可通过opChar边到达别的状态
                if(m_DFAStateArr[state].DFAEdgeMap.contains(opChar))
                {
                    int id=getStateId(m_dividedSet,m_DFAStateArr[state].DFAEdgeMap[opChar]);
                    //若该边不存在，插入边（完善mDFA边信息）
                    if(!m_mDFANodeArr[i].mDFAEdgesMap.contains(opChar))
                    {
                        m_mDFANodeArr[i].mDFAEdgesMap[opChar]=id;
                    }
                }
            }
        }
    }
}

/**
 * @brief NDFA::mDFA2Lexer
 * @return Lexer
 * 根据最小化DFA，生成词法分析程序C语言代码，返回代码字符串
 */
QString NDFA::mDFA2Lexer(QString filePath)
{
    QStringList keywordList=m_reg_keyword_str.split('|');

    QString lexCode;
    int m_state=m_mDFAG.startState;//最小化DFA的初态

    //库函数
    lexCode+="#include<stdio.h>\n"
             "#include<stdlib.h>\n"
             "#include<string.h>\n"
             "#include<ctype.h>\n"
            "#include<set>\n"
             "#include<unordered_map>\n";
    //关键字映射map
    lexCode+="std::set<std::string> keywordSet={};\n";

    //生成分析代码
    lexCode+="void coding(FILE* input_fp,FILE* output_fp) {\n"
             "\tchar tmp = fgetc(input_fp);\n"
             "\tif (tmp == ' ' || tmp == '\\n' || tmp == '\\t'){\n"
             "\t\tfprintf(output_fp, \"%c\", tmp);\n"
             "\t\tprintf(\"%c\", tmp);\n"
             "\t\treturn;\n"
             "\t}\n"
             "\tungetc(tmp, input_fp);\n"
             "\tint state = "+QString::number(m_state)+";\n"
             "\tbool flag = false;\n"
             "\tbool isIdentifier = false;\n"
            "\tbool isDigit = false;\n"
             "\tbool isAnnotation = false;\n"
             "\tstd::string value;\n"
             "\twhile (!flag) {\n"
             "\t\ttmp = fgetc(input_fp);\n"
             "\t\tswitch (state) {\n";

    for(int i=0;i<m_mDFAStateNum;i++)
    {
        if(m_mDFANodeArr[i].mDFAEdgesMap.size()){
            lexCode+="\t\tcase "+QString::number(i)+": {\n";
            lexCode+="\t\t\tswitch (tmp) {\n";
            QList<QString> tmpList=m_mDFANodeArr[i].mDFAEdgesMap.keys();//该状态的所有边值
            if(genLexCase(tmpList,lexCode,i,1))
                lexCode+="\t\t\tdefault:state = "+QString::number(m_mDFANodeArr[i].mDFAEdgesMap["~"])+"; isAnnotation = true; break;\n";
            lexCode+="\t\t\t}\n";
            lexCode+="\t\t\tbreak;\n";
            lexCode+="\t\t}\n";
        }
    }
    lexCode+="\t\t}\n";
    lexCode+="\t\tvalue += tmp;\n";

    QList<int> stateList=m_mDFAG.endStateSet.values();//所有终态

    lexCode+="\t\tif (";
    for(int i=0;i<stateList.size();i++)
    {
        //要提前读一个字符判断是不是真的到终态，因为到了终态不一定是真正的终态
        int num=stateList[i];
        if(i)lexCode+="\t\telse if (";
        lexCode+="state =="+QString::number(num)+") {\n";
        lexCode+="\t\t\ttmp = fgetc(input_fp);\n";
        lexCode+="\t\t\tswitch (tmp) {\n";
        QList<QString> tmpList=m_mDFANodeArr[num].mDFAEdgesMap.keys();//该状态的所有边值
        genLexCase(tmpList,lexCode,num,0);
        lexCode+="\t\t\tdefault: {\n";
        lexCode+="\t\t\t\tflag=true;\n";
        if(tmpList.contains("letter"))
            lexCode+="\t\t\t\tisIdentifier = true;\n";
        lexCode+="\t\t\t}\n"
                 "\t\t\t}\n"
                 "\t\t\tungetc(tmp, input_fp);\n"
                 "\t\t}\n";
    }
    lexCode+="\t}\n";

    //为适配解码增加Keyword:前缀，数字Digit:前缀，ID:标识符前缀

    lexCode+="\tif (keywordSet.count(value)) {\n"
            "\t\tfprintf(output_fp, \"Keyword:%s \", value.c_str());\n"
            "\t\tprintf(\"Keyword:%s \", value.c_str());\n"
            "\t\treturn;\n"
            "\t}\n"
            "\tif (isIdentifier) {\n"
            "\t\tfprintf(output_fp, \"ID:%s \", value.c_str());\n"
            "\t\tprintf(\"ID:%s \", value.c_str());\n"
            "return;\n"
            "\t}\n"
            "\tif (isDigit) {\n"
            "\t\tfprintf(output_fp, \"Digit:%s \", value.c_str());\n"
            "\t\tprintf(\"Digit:%s \", value.c_str());\n"
            "\t\treturn;\n"
            "\t}\n"
            "\tif (!isAnnotation) {\n"
            "\t\tfprintf(output_fp, \"%s \", value.c_str());\n"
            "\t\tprintf(\"%s \", value.c_str());\n"
            "\t}\n"
            "};\n";

    //主函数
    QFileInfo fileInfo(filePath);
    QString t_tmpFilePath=fileInfo.path();
    lexCode+="int main(int argc, char* argv[]) {\n"
             "\tFILE* input_fp = fopen(\""+filePath+"/_sample.tny\", \"r\");\n"
             "\tif (input_fp == NULL) {\n"
             "\t\tprintf(\"Failed to open input file\");\n"
             "\t\treturn 1;\n"
             "\t}\n";

    lexCode+="\tFILE* output_fp = fopen(\""+t_tmpFilePath+"/output.lex"+"\", \"w\");\n"
             "\tif (output_fp == NULL) {\n"
             "\t\tprintf(\"Failed to open output file\");\n"
             "\t\tfclose(input_fp);\n"
             "\t\treturn 1;\n"
             "\t}\n";


    lexCode+="keywordSet = { ";
    for(const auto &tmp : keywordList)
    {
        lexCode+="\""+tmp+"\",";
    }
    lexCode.chop(1);
    lexCode+=" };\n";

    lexCode+="\tchar c;\n"
             "\twhile ((c=fgetc(input_fp)) != EOF) {\n"
             "\t\tungetc(c, input_fp);\n"
             "\t\tcoding(input_fp, output_fp);\n"
             "\t}\n"
             "\tfclose(input_fp);\n"
             "\tfclose(output_fp);\n"
             "\treturn 0;\n"
             "}";

    m_lexerCodeStr=lexCode;
    return lexCode;
}

void NDFA::setPath(QString srcFilePath, QString tmpFilePath)
{
    this->m_srcFilePath=srcFilePath;//源程序文件路径
    this->m_tmpFilePath=tmpFilePath;
}

void NDFA::setKeywordStr(QString kStr)
{
    this->m_reg_keyword_str=kStr;
}
