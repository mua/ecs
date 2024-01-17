{
    "vertex": {
        "in": {
            "attributes": {
                "position": "vec3"
            },
            "uniforms": {
                "model": "mat4",
                "view": "mat4",
                "projection": "mat4"
            }
        },
        "out": {
            "gl_Position": "vec4"
        },
        "code": "\
            gl_position=
        "
    }
}