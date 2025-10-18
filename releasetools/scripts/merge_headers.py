#!/usr/bin/env python3
import os
import re
import sys
from collections import defaultdict, deque

# 尝试导入 graphviz，失败则禁用绘图
try:
    import graphviz
    HAS_GRAPHVIZ = True
except ImportError:
    HAS_GRAPHVIZ = False
    print("提示: 未安装 graphviz 库，将仅输出 .dot 源文件（可手动渲染）", file=sys.stderr)

def parse_local_includes(filepath):
    includes = []
    include_re = re.compile(r'^\s*#\s*include\s+"([^"]+)"')
    try:
        with open(filepath, 'r', encoding='utf-8-sig', errors='ignore') as f:
            for line in f:
                match = include_re.match(line)
                if match:
                    includes.append(match.group(1))
    except Exception as e:
        print(f"警告: 无法读取文件 {filepath}: {e}", file=sys.stderr)
    return includes

def build_dependency_graph(header_dir, all_headers):
    header_set = set(all_headers)
    graph = defaultdict(list)      # dep -> [users]
    edges = []                     # 用于绘图: (from, to)
    in_degree = {h: 0 for h in all_headers}

    for h in all_headers:
        graph[h] = []

    for h in all_headers:
        full_path = os.path.join(header_dir, h)
        deps = parse_local_includes(full_path)
        for dep in deps:
            if dep in header_set:
                graph[dep].append(h)
                in_degree[h] += 1
                edges.append((dep, h))  # dep -> h

    return graph, in_degree, edges

def topological_sort(graph, in_degree, headers):
    queue = deque([h for h in headers if in_degree[h] == 0])
    result = []
    while queue:
        node = queue.popleft()
        result.append(node)
        for neighbor in graph[node]:
            in_degree[neighbor] -= 1
            if in_degree[neighbor] == 0:
                queue.append(neighbor)
    return result

def write_dot_file(dot_path, all_headers, edges, safe_nodes=None):
    """输出 .dot 文件"""
    with open(dot_path, 'w', encoding='utf-8-sig') as f:
        f.write('digraph HeaderDependencies {\n')
        f.write('    rankdir=LR;\n')
        f.write('    node [shape=box, style=filled, fillcolor=lightblue];\n')

        safe_set = set(safe_nodes) if safe_nodes else set()

        # 写入节点
        for h in all_headers:
            if safe_nodes is not None and h not in safe_set:
                f.write(f'    "{h}" [fillcolor=red, fontcolor=white];\n')
            else:
                f.write(f'    "{h}";\n')

        # 写入边
        for src, dst in edges:
            if safe_nodes is not None and (src not in safe_set or dst not in safe_set):
                f.write(f'    "{src}" -> "{dst}" [color=red, fontcolor=red];\n')
            else:
                f.write(f'    "{src}" -> "{dst}";\n')

        f.write('}\n')
    print(f"依赖图已保存至: {dot_path}")

def render_dot_with_graphviz(dot_path, output_png):
    if not HAS_GRAPHVIZ:
        return
    try:
        with open(dot_path, 'r', encoding='utf-8-sig') as f:
            dot_data = f.read()
        g = graphviz.Source(dot_data)
        g.render(filename=output_png.replace('.png', ''), format='png', cleanup=True)
        print(f"可视化图像已生成: {output_png}")
    except Exception as e:
        print(f"警告: 无法渲染 PNG 图像（请确保安装了 Graphviz）: {e}", file=sys.stderr)

def merge_headers(header_dir, output_file):
    all_headers = [f for f in os.listdir(header_dir) if f.endswith('.h')]
    if not all_headers:
        print("错误: 指定目录中没有 .h 文件", file=sys.stderr)
        sys.exit(1)

    pch_file = 'pch.h'
    has_pch = pch_file in all_headers

    # 构建图
    graph, in_degree, edges = build_dependency_graph(header_dir, all_headers)
    topo_order = topological_sort(graph, in_degree.copy(), all_headers)

    # 检测是否完整
    success = (len(topo_order) == len(all_headers))
    safe_nodes = set(topo_order) if not success else None

    # 输出依赖图
    dot_path = output_file.replace('.h', '_deps.dot')
    write_dot_file(dot_path, all_headers, edges, safe_nodes if not success else None)
    if success:
        png_path = output_file.replace('.h', '_deps.png')
        render_dot_with_graphviz(dot_path, png_path)

    # 合并头文件
    included = set()
    with open(output_file, 'w', encoding='utf-8-sig') as out_f:
        if has_pch:
            out_f.write('// === Merged from: pch.h (precompiled header) ===\n')
            with open(os.path.join(header_dir, pch_file), 'r', encoding='utf-8-sig', errors='ignore') as f:
                out_f.write(f.read())
            out_f.write('\n')
            included.add(pch_file)

        include_quote_re = re.compile(r'^(\s*)#\s*include\s+"([^"]+)"(.*)$')
        for h in topo_order:
            if h in included:
                continue
            full_path = os.path.join(header_dir, h)
            try:
                with open(full_path, 'r', encoding='utf-8-sig', errors='ignore') as f:
                    lines = f.readlines()
            except Exception as e:
                print(f"错误: 无法读取 {h}: {e}", file=sys.stderr)
                continue

            out_f.write(f'\n// === Merged from: {h} ===\n')
            for line in lines:
                match = include_quote_re.match(line)
                if match:
                    indent = match.group(1)
                    include_content = f'#include "{match.group(2)}"{match.group(3)}'
                    out_f.write(f'{indent}// {include_content}\n')
                else:
                    out_f.write(line)
            included.add(h)

    print(f"成功合并 {len(included)} 个头文件到 {output_file}")
    if not success:
        missing = set(all_headers) - set(topo_order)
        print(f"警告: 存在循环依赖！以下文件未被包含: {missing}")
        print(f"   请检查 {dot_path} 中标红的节点和边")

if __name__ == '__main__':
    if len(sys.argv) != 3:
        print("用法: python merge_headers.py <头文件目录> <输出文件路径>")
        sys.exit(1)
    header_dir = sys.argv[1]
    output_file = sys.argv[2]
    if not os.path.isdir(header_dir):
        print(f"错误: {header_dir} 不是一个有效目录")
        sys.exit(1)
    merge_headers(header_dir, output_file)