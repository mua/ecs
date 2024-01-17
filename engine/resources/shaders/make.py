from yaml import load, dump
import os
import pprint
import re
import json

dir = os.path.abspath(os.path.dirname(__file__))
fname = os.path.join(dir, "basic.shader.yaml")

with open(fname) as f:
    shader_list = load(f)

shaders = []
pprint.pprint(shaders)

def make_unique(vars):
    uniq_vars = []
    for var in vars:
        if var not in uniq_vars:
            uniq_vars.append(var)
    return uniq_vars

def extract_list(v):
    ret = []
    for e in v or []:
        ret.append(list(e.items())[0])
    return ret

def output_shader(shaders, stage, attrs, uniforms, vars, outs):
    src = []
    attr_src = []
    uniform_src = []
    var_src = []
    decl_src = []

    if stage == "vertex":
        for i, attr in enumerate(attrs):
            attr_src.append("layout(location = %s) in %s;" % (attr.loc, attr.declaration()))
    attr_src = "\n".join(attr_src)

    for i, uniform in enumerate(uniforms):
        uniform_src.append("uniform %s;" % (uniform.declaration()))
    uniform_src = "\n".join(uniform_src)

    for i, var in enumerate(vars):
        if not var.name.startswith("gl_"):
            if stage == "vertex":
                var_src.append("out %s;" % (var.declaration()))
            else:
                var_src.append("in %s;" % (var.declaration()))        
    
    for i, var in enumerate(outs):    
        if stage != "vertex":
            var_src.append("layout(location = %s) out %s;" % (i, var.declaration()))

    var_src = "\n".join(var_src)            

    var_names = [var.name for var in vars+outs]

    for shader in shaders:
        for out in shader.outputs:
            if out.name not in var_names:                
                var_names.append(out.name)
                decl_src.append("%s;" % out.declaration())
    decl_src = "\n".join(decl_src)

    for vert in shaders:
        src.append("//\t***********%s**********" % vert.name)
        for l in vert.source().split("\n"):
            src.append("\t" + l.strip())
    src = "\n".join(src)
    wrapped = """
#version 300 es
precision mediump float;    

%s

%s

%s

void main()
{
%s

%s
}
    """ % (
        attr_src,
        uniform_src,
        var_src,
        decl_src,
        src,
    )
    print(wrapped)
    wrapped = wrapped.strip()
    return wrapped


class Slot:
    def __init__(self, node, name, type):
        self.node = node
        self.type = type
        self.name = name
        self.count = 1
        self.loc = -1
        arr = re.findall("([a-zA-Z0-9]+)\[\s*([0-9]+)\s*\]$", type)
        if arr:
            self.type, count = arr[0]
            self.count = int(count)

    def __eq__(self, other):
        return self.name == other.name and self.type == other.type

    def declaration(self):
        return (
            "%s %s" % (self.type, self.name)
            if self.count == 1
            else "%s %s[%s]" % (self.type, self.name, self.count)
        )

class NodeLink:
    def __init__(self, source, target):
        self.source = source  # node
        self.target = target


class Node:
    def __init__(self, name, stage, requires, provides, input, output, attrs = {}, uniforms={}, glsl=""):
        self.name = name
        self.deps = []
        self.inputs = {}

        self.requires = requires or []
        self.provides = provides or []
        self.stage = stage

        self.outputs = []
        for k, v in extract_list(output):
            self.outputs.append(Slot(self, k, v))
        if input:
            for k, v in extract_list(input):
                self.inputs[k] = Slot(self, k, v)

        self.attributes = [
            Slot(self, k, v)
            for k, v in extract_list(attrs)
        ]
        self.uniforms = [
            Slot(self, k, v) for k, v in extract_list(uniforms)
        ]

        self.glsl = glsl

    def source(self):
        return self.glsl

    def __str__(self):
        ret = "\n%s:" % self.name
        for c in self.deps:
            ret += "\n".join(["\t" + line for line in str(c).split("\n")])
        return ret   

    def output_by_name(self, n):
        for out in self.outputs:
            if out.name == n:
                return out            


class Output3(Node):
    def __init__(self, requires):
        super().__init__("output", "fragment", requires, [], {
            "screenPosition": "vec4",
            "clipVertexNormal": "vec3"
        }, {}, glsl = "gl_FragColor = vec4(clipVertexNormal, 1.0);")


class Output(Node):
    def __init__(self, requires, output, output_map):        
        super().__init__("output", "fragment", requires, [], [], output = output)
        self.output_map = output_map

    def source(self):
        vars = {}
        for k, v in self.output_map.items():
            for dep in self.deps:
                if dep.output_by_name(v):
                    vars[k] = dep.output_by_name(v)
        lines = []
        for k,v in vars.items():
            if v.type == "vec4":
                lines.append("%s = %s;" % (k, v.name))
            elif v.type == "vec3":
                lines.append("%s = vec4(%s, 1.0);" % (k, v.name))
        return "\n".join(lines)


class Output2(Node):
    def __init__(self, requires):
        super().__init__("output", "fragment", requires, [], {
            "gl_FragColor": "vec4"
        }, {})


repository = []

for name, shader in shader_list.items():
    node = Node(
        name,
        shader.get("stage"),
        shader.get("requires"),
        shader.get("provides"),
        shader.get("in", {}).get("vars"),
        shader.get("out"),
        shader.get("in", {}).get("attributes", []),
        shader.get("in", {}).get("uniforms", []),
        "\n".join([l.strip() for l in shader.get("glsl", "").split("\n")])
    )
    repository.append(node)


def resolve(output, repository):
    def get_node_provides(req):
        for src in repository:
            if req in src.provides:
                return src

    for trg in repository:
        trg.deps = []
        for req in trg.requires:
            trg.deps.append(get_node_provides(req))

    path = []
    queue = [output]
    while queue:
        node = queue.pop(0)
        path.append(node)
        for dep in node.deps:
            if dep in path:
                path.remove(dep)
            queue.append(dep)
    path.reverse()    
    return path


def compile(path):
    verts = [a for a in filter(lambda x: x.stage == "vertex", path)]
    frags = [a for a in filter(lambda x: x.stage == "fragment", path)]

    print("\nverts:")
    print("\n".join([n.name for n in verts]))
    print("\nfrags:")
    print("\n".join([n.name for n in frags]))

    attrs = []
    uniforms = []
    vars = []
    outs = []

    for shader in path:
        attrs += shader.attributes
        uniforms += shader.uniforms
        if shader.stage == "vertex":
            vars += shader.outputs

    outs += path[-1].outputs

    vars = make_unique(vars)
    outs  = make_unique(outs)
    attrs = make_unique(attrs)
    uniforms = make_unique(uniforms)

    attr_sort = ['position', 'color', 'normal', 'uv']
    attrs.sort(key = lambda a: attr_sort.index(a.name) if a.name in attr_sort else -1)    

    shader_meta = {"attributes": [], "uniforms": []}
    for attr in attrs:
        shader_meta["attributes"].append({"name": attr.name, "type": attr.type})
        attr.loc = attr_sort.index(attr.name)
    for uniform in uniforms:
        shader_meta["uniforms"].append({"name": uniform.name, "type": uniform.type})

    return (
        output_shader(verts, "vertex", attrs, uniforms, vars, outs),
        output_shader(frags, "fragment", attrs, uniforms, vars, outs),
        json.dumps(shader_meta, indent=True)
    )


#path = resolve(output)
#compile(path)

def output_graph(repository, path, name):
    from graphviz import Digraph

    dot = Digraph()
    dot.attr("node", shape="record")

    for node in repository:
        dot.node(
            node.name,
            label="{%s|%s}" % (node.name, node.stage),
            color="red" if node in path else "black",
        )

        for dep in node.deps:
            dot.edge(node.name, dep.name, color = "blue", style="dashed")                   

    fn = os.path.join(dir, "graphs", "%s.gv" % name)
    dot.render(fn)

def build(features, name, output, out_map):
    output = Output(features, output, out_map)
    repo = repository[:]
    repo.append(output)
    path = resolve(output, repo)

    vert, frag, meta = compile(path)
    
    output_graph(repo, path, name)

    with open(os.path.join(dir, "%s.vert" % name), "w") as f:
        f.write(vert)

    with open(os.path.join(dir, "%s.frag" % name), "w") as f:
        f.write(frag)

    with open(os.path.join(dir, "%s.json" % name), "w") as f:
        f.write(meta)


#build(["ssaoGen"], "ssao2",  [{"gColor": "vec4"}], {"gColor": "color"})
#build(["transform", "ssao", "texturedMaterialColor"], "textured.material",  [{"gColor": "vec4"}], {"gColor": "color"})
#build(["transform", "clipVertexNormal"], "gbuf",  [{"gPosition": "vec4"}, {"gNormal": "vec4"}], {"gNormal": "pVertexNormal", "gPosition": "pPosition"})
build(["transform", "materialColor"], "basic.color",  [{"gColor": "vec4"}], {"gColor": "color"})