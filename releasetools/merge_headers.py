#!/usr/bin/env python3
import os
import re
import sys
from enum import Enum
import argparse
from collections import deque

# 尝试导入 graphviz，失败则禁用绘图
try:
    import graphviz
    HAS_GRAPHVIZ = True
except ImportError:
    HAS_GRAPHVIZ = False
    print("提示: 未安装 graphviz 库，将仅输出 .dot 源文件（可手动渲染）", file=sys.stderr)


# 全局参数
mainRoot = ''
roots = []
output = ""
antiCollisionMarcoDefine = False

# 全局变量
headers = []
sortedHeaders = []

# debug标志
debug = False



class header:
    class header_type(Enum):
        main = 0        # 主要头文件
        rely = 1        # 依赖库头文件
        missing = 2     # 缺失
        error = 3       # 错误，主要是循环依赖

    def __init__(self,nameWithPath,type):
        self.nameWithPath = nameWithPath
        self.type = type
        self.reliedBy = []     # 被依赖的列表
        self.reliedByCount = 0 
        self.relyOn = []       # 依赖的列表
        self.relyOnCount = 0
        self.includeLineTobeDelete = []  # 用于后续处理，将include行注释掉 
        
    def __eq__(self, value):
        if isinstance(value,header):
            return self.nameWithPath == value.nameWithPath
        if isinstance(value,str):
            return self.nameWithPath == value
        return False
    
    def __hash__(self):
        return hash(self.nameWithPath)
    
    def relied_by(self,d):
        self.reliedBy.append(d)
        self.reliedByCount += 1

    def rely_on(self,d):
        self.relyOn.append(d)
        self.relyOnCount += 1
    
    def parse_include(self):
        global mainRoot
        global roots
        global headers
        global debug
        include_re = re.compile(r'^(\s*#\s*include\s*"(.*?)".*?)$')
        try:
            with open(self.nameWithPath,'r',encoding='utf-8-sig') as f:
                for line_num, line in enumerate(f):
                    m = include_re.match(line)
                    if m:
                        fullLine = m.group(1)                   # 整行
                        relPath = m.group(2).replace('\\','/')  # include 文件名
                        found = False
                        # 先从本文件路径中寻找
                        phyPath = os.path.dirname(self.nameWithPath) + os.path.sep + relPath
                        if os.path.exists(phyPath) and os.path.isfile(phyPath):
                            found = True
                        # 再从root路径中挨个查找
                        else:
                            for root in roots:
                                phyPath = root + os.path.sep + relPath
                                if os.path.exists(phyPath) and os.path.isfile(phyPath):
                                    found = True
                                    break
                        if found:
                            if debug:
                                print(f'文件 {self.nameWithPath} 第 {line_num} 行语句 {fullLine} 找到匹配项 {phyPath}')
                            if phyPath not in headers:  # 列表中没有该头文件，则添加
                                headers.append(header(phyPath,header.header_type.rely))
                            self.includeLineTobeDelete.append(fullLine)
                            idx = headers.index(phyPath)
                            headers[idx].relied_by(self)
                            self.rely_on(headers[idx])
                        else:
                            if relPath not in headers:  # 列表中没有，则添加
                                headers.append(header(relPath,header.header_type.missing))
                            print(f'警告：未找到头文件：{relPath}，在文件 {self.nameWithPath} 中第 {line_num} 行：{fullLine}')
                            idx = headers.index(relPath)
                            headers[idx].relied_by(self)
                            self.rely_on(headers[idx])
        except Exception as e:
            print(f'警告：无法打开文件{self.nameWithPath}: {e}')


# 解析依赖关系：遍历mainRoot，将其中的头文件和所需的头文件添加至列表中
def parse_rely():
    global mainRoot
    global roots
    global headers
    global debug
    if not os.path.isdir(mainRoot):
        print(f'错误：主根 {mainRoot} 不存在')
    # 扫描主根
    for dirpath, _, filenames in os.walk(mainRoot):
        for f in filenames:
            if f.endswith('.h'):
                phyPath = os.path.join(dirpath,f)
                headers.append(header(phyPath,header.header_type.main))
                if debug:
                    print(f'在主根中找到文件：{phyPath}')
    # 列表逐个解析
    for h in headers:
        h.parse_include()




# 依赖分析：解析依赖关系
def sort_headers():
    global headers
    global sortedHeaders
    queue = deque([n for n in headers if n.relyOnCount == 0])
    while queue:
        n = queue.popleft()
        sortedHeaders.append(n)
        for s in n.reliedBy:
            s.relyOnCount -= 1
            if s.relyOnCount == 0:
                queue.append(s)
        if debug:
            print(f'文件 {n.nameWithPath} 加入结果队列')
    for n in headers:
        if n.relyOnCount > 0:
            n.type = header.header_type.error
            print(f'警告：文件 {n.nameWithPath} 存在循环依赖项, 将不会被添加到最终结果中')
        


# 生成依赖图
def wirte_dot_file(dotPath):
    global mainRoot
    global roots
    global headers
    global debug
    try:
        with open(dotPath+'.dot','w',encoding='utf-8-sig') as f:
            f.write('digraph HeaderDependencies {\n')
            f.write('   rankdir=LR;\n')
            f.write('   node [style=filled,fillcolor=white];\n')
            # 绘制节点
            for h in headers:
                attr = ''
                if h.type == header.header_type.main:
                    attr = 'fillcolor=lightgreen'
                elif h.type == header.header_type.rely:
                    attr = 'fillcolor=lightblue'
                elif h.type == header.header_type.missing:
                    attr = 'fillcolor=pink'
                elif h.type == header.header_type.error:
                    attr = 'fillcolor=red'
                f.write(f'  "{os.path.basename(h.nameWithPath)}" [{attr}];\n')
            # 绘制边
            for h in headers:
                attr = ''
                if h.type == header.header_type.error:
                    attr = 'color=red'
                else:
                    attr = 'color=lightgreen'
                for d in h.reliedBy:
                    f.write(f'  "{os.path.basename(h.nameWithPath)}" -> "{os.path.basename(d.nameWithPath)}" [{attr}];\n')
            f.write('}\n')
        dotFilePath = dotPath + '.dot'
        print(f'依赖图文件保存至：{dotFilePath}')
        if HAS_GRAPHVIZ:
            with open(dotPath+'.dot','r',encoding='utf-8-sig') as f:
                dotData = f.read()
            g = graphviz.Source(dotData)
            g.render(filename=dotPath,format='png',cleanup=True)
            dotImgPath = dotPath + '.png'
            print(f'依赖图已生成：{dotImgPath}')
    except Exception as e:
        print(f'创建依赖图失败：{e}')


def upper_marco_for_header(headerFullPath):
    res = '_HYDRA15_' + os.path.basename(os.path.dirname(headerFullPath)).replace('.','_').upper() + '_'
    if os.path.isfile(headerFullPath):
        res = res + os.path.basename(headerFullPath).replace('.','_').upper() + '_'
    return res

def generate_header_file(output):
    global sortedHeaders
    global debug
    with open(output,'w',encoding='utf-8-sig') as f:
        f.write('#pragma once\n')
        f.write(f'#ifndef {upper_marco_for_header(mainRoot)}\n#define {upper_marco_for_header(mainRoot)}\n')
        for h in sortedHeaders:
            if not (h.type == header.header_type.main or h.type == header.header_type.rely):
                continue
            content = f'\n\n\n/*************** 合并自 {os.path.basename(h.nameWithPath)} ***************/\n'
            content += f'#ifndef {upper_marco_for_header(h.nameWithPath)}\n#define {upper_marco_for_header(h.nameWithPath)}\n'
            with open(h.nameWithPath,'r',encoding='utf-8-sig') as s:
                content += s.read()
                content = content.replace('#pragma once','// #pragma once')
                for r in h.includeLineTobeDelete:
                    if debug:
                        print(f'替换include行：{r}')
                    content = content.replace(r,f'// {r}')
            content += '\n#endif\n'
            f.write(content)
            if debug:
                print(f'已将文件 {h.nameWithPath} 添加到合并结果中')
        f.write('#endif')
    print(f'已完成合并，共合并 {len([i for i in sortedHeaders if (i.type == header.header_type.main or i.type == header.header_type.rely)])} 个文件')




def main():
    argParser = argparse.ArgumentParser(description='合并c++头文件：主根目录中的所有文件均合并，根目录中的文件按需合并')
    argParser.add_argument('-o','--output',required=True,help='输出文件名')
    argParser.add_argument('-m','--main-root',required=True,help='主根目录')
    argParser.add_argument('-r','--root',nargs='+',required=False,help='根目录')
    argParser.add_argument('-c','--anticollision-marco',action='store_true',help='启用防冲突宏定义')

    args = argParser.parse_args()

    global mainRoot
    global roots
    global output
    global antiCollisionMarcoDefine
    mainRoot = os.path.abspath(args.main_root.replace('\\','/'))
    if args.root:
        roots += [os.path.abspath(i.replace('\\','/')) for i in args.root]
    output = args.output
    antiCollisionMarcoDefine = args.anticollision_marco


    if debug:
        print(f'使用主根目录：{mainRoot}')
        print(f'使用根目录：{roots}')
        print(f'使用输出文件：{output}')
        print('防冲突宏定义已启用' if antiCollisionMarcoDefine else '防冲突宏定义未启用')
    
    parse_rely()
    sort_headers()
    wirte_dot_file(output+'_dependency_map')
    generate_header_file(output)

    


if __name__ == '__main__':
    main()
