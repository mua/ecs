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


class ShaderVar:
    def __init__(self, name, type):
        self.name = name
        self.type = type
        self.count = 1
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


class ShaderNode(object):
    def __init__(self, name, shader):
        self.deps = []
        self.requires = shader.get("requires", [])
        self.shader = shader
        self.provides = shader.get("provides", [])
        self.name = name
        self.stage = shader["stage"]

        self.attributes = [
            ShaderVar(k, v)
            for k, v in shader.get("in", {}).get("attributes", {}).items()
        ]
        self.uniforms = [
            ShaderVar(k, v) for k, v in shader.get("in", {}).get("uniforms", {}).items()
        ]
        self.outs = [ShaderVar(k, v) for k, v in shader.get("out", {}).items()]

        self.glsl = "\n".join([l.strip() for l in shader.get("glsl", "").split("\n")])

    def __str__(self):
        ret = "\n%s:" % self.name
        for c in self.deps:
            ret += "\n".join(["\t" + line for line in str(c).split("\n")])
        return ret

    def variable_name(link):
        return link

    def source(self):
        return self.glsl


for name, shader in shader_list.items():
    node = ShaderNode(name, shader)
    shaders.append(node)


def find_shader(lmb):
    for shader in shaders:
        if lmb(shader):
            return shader


def shaders_by_effects(effects):
    for effect in effects:
        shader = find_shader(lambda s: effect in s.provides)
        if shader:
            return shader


def satisfy(node):
    for req in node.requires:
        cnode = shaders_by_effects([req])
        if not cnode:
            print("Cannot satisfy %s" % req)
            return
        satisfy(cnode)
        if cnode not in node.deps:
            node.deps.append(cnode)


def resolve(effects):
    root = ShaderNode("output", {"requires": effects, "stage": "fragment"})
    satisfy(root)
    return root


def make_unique(vars):
    uniq_vars = []
    for var in vars:
        if var not in uniq_vars:
            uniq_vars.append(var)
    return uniq_vars


def compile(node):
    lst = []
    path = [node]
    current = 0
    while len(path) > current:
        cn = path[current]
        for dep in cn.deps:
            if dep in path:
                if path.index(dep) < current:
                    current -= 1
                path.remove(dep)
            path.append(dep)
        current += 1
    path.reverse()

    verts = [a for a in filter(lambda x: x.stage == "vertex", path)]
    frags = [a for a in filter(lambda x: x.stage == "fragment", path)]

    print("\nverts:")
    print("\n".join([n.name for n in verts]))
    print("\nfrags:")
    print("\n".join([n.name for n in frags]))

    print("vertex glsl")

    attrs = []
    uniforms = []
    vars = []

    for shader in path:
        attrs += shader.attributes
        uniforms += shader.uniforms
        if shader.stage == "vertex":
            vars += shader.outs

    vars = make_unique(vars)
    attrs = make_unique(attrs)
    uniforms = make_unique(uniforms)

    return (
        output(verts, "vertex", attrs, uniforms, vars),
        output(frags, "fragment", attrs, uniforms, vars),
    )


def output_shader(shaders, stage, attrs, uniforms, vars):
    src = []
    attr_src = []
    uniform_src = []
    var_src = []
    decl_src = []

    if stage == "vertex":
        for i, attr in enumerate(attrs):
            attr_src.append("layout(location = %s) in %s;" % (i, attr.declaration()))
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
    var_src = "\n".join(var_src)

    var_names = [var.name for var in vars]
    for shader in shaders:
        for out in shader.outputs.values():
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


def build(features, name):
    root = resolve(features)
    print(root)
    vert, frag = compile(root)

    with open(os.path.join(dir, "%s.vert" % name), "w") as f:
        f.write(vert)

    with open(os.path.join(dir, "%s.frag" % name), "w") as f:
        f.write(frag)


# build(["litMaterialColor", "transform"], "lit")
# build(["transform"], "drawonly")
# build(["skinning", "transform"], "skinned")
# build(["transform", "vertexColor"], "vertexColor")


class Slot:
    def __init__(self, node, name, type):
        self.node = node
        self.type = type
        self.name = name
        self.count = 1
        arr = re.findall("([a-zA-Z0-9]+)\[\s*([0-9]+)\s*\]$", type)
        if arr:
            self.type, count = arr[0]
            self.count = int(count)

    # def __eq__(self, other):
    #     return self.name == other.name and self.type == other.type

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

        self.outputs = {}
        for k, v in output.items():
            self.outputs[k] = Slot(self, k, v)
        if input:
            for k, v in input.items():
                self.inputs[k] = Slot(self, k, v)

        self.attributes = [
            Slot(self, k, v)
            for k, v in attrs.items()
        ]
        self.uniforms = [
            Slot(self, k, v) for k, v in uniforms.items()
        ]

        self.glsl = glsl

    def source(self):
        return self.glsl

    def __str__(self):
        ret = "\n%s:" % self.name
        for c in self.deps:
            ret += "\n".join(["\t" + line for line in str(c).split("\n")])
        return ret   


class Output3(Node):
    def __init__(self, requires):
        super().__init__("output", "fragment", requires, [], {
            "screenPosition": "vec4",
            "clipVertexNormal": "vec3"
        }, {}, glsl = "gl_FragColor = vec4(clipVertexNormal, 1.0);")

class Output(Node):
    def __init__(self, requires):
        super().__init__("output", "fragment", requires, [], {
            "color": "vec4"
        }, {}, glsl = "gl_FragColor = color;")


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
        shader.get("in", {}).get("attributes", {}),
        shader.get("in", {}).get("uniforms", {}),
        "\n".join([l.strip() for l in shader.get("glsl", "").split("\n")])
    )
    repository.append(node)


def resolve(output, repository):
    links = []

    def get_slot_link(slot):
        for link in links:
            if link.target == slot:
                return slot

    def connect(a, b):
        for k, output in b.outputs.items():
            if k in a.inputs:
                if not get_slot_link(a.inputs[k]):
                    links.append(NodeLink(output, a.inputs[k]))
    
    def links_to_node(node):
        return [link for link in links if link.target.node == node]

    for trg in repository:
        for req in trg.requires:
            for src in repository:
                if src != trg and req in src.provides:
                    connect(trg, src)
                    trg.deps.append(src)

    path = []
    queue = [output]
    while queue:
        node = queue.pop(0)
        path.append(node)
        for link in links_to_node(node):
            if link.source in path:
                path.remove(link.source.node)
            queue.append(link.source.node)
    path.reverse()
    return path, links


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

    for shader in path:
        attrs += shader.attributes
        uniforms += shader.uniforms
        if shader.stage == "vertex":
            vars += shader.outputs.values()

    vars = make_unique(vars)
    attrs = make_unique(attrs)
    uniforms = make_unique(uniforms)

    attr_sort = ['position', 'color', 'normal', 'uv']
    attrs.sort(key = lambda a: attr_sort.index(a.name) if a.name in attr_sort else -1)    

    shader_meta = {"attributes": [], "uniforms": []}
    for attr in attrs:
        shader_meta["attributes"].append({"name": attr.name, "type": attr.type})
    for uniform in uniforms:
        shader_meta["uniforms"].append({"name": uniform.name, "type": uniform.type})

    return (
        output_shader(verts, "vertex", attrs, uniforms, vars),
        output_shader(frags, "fragment", attrs, uniforms, vars),
        json.dumps(shader_meta, indent=True)
    )


#path = resolve(output)
#compile(path)

def output_graph(repository, path, links, name):
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

    for link in links:
        dot.edge(link.target.node.name, link.source.node.name)            

    fn = os.path.join(dir, "graphs", "%s.gv" % name)
    dot.render(fn)

def build(features, name):
    output = Output(features)
    repo = repository[:]
    repo.append(output)
    path, links = resolve(output, repo)

    vert, frag, meta = compile(path)
    
    output_graph(repo, path, links, name)

    with open(os.path.join(dir, "%s.vert" % name), "w") as f:
        f.write(vert)

    with open(os.path.join(dir, "%s.frag" % name), "w") as f:
        f.write(frag)

    with open(os.path.join(dir, "%s.json" % name), "w") as f:
        f.write(meta)


#build(["transform", "vertexColor"], "basic")
#build(["transform", "texturedMaterialColor"], "textured.material")
build(["transform", "clipVertexNormal"], "gbuf")