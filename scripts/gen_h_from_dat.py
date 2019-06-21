# greeting.py
#
# Demonstration of the pyparsing module, on the prototypical "Hello, World!"
# example
#
# Copyright 2003, 2019 by Paul McGuire
#
import pyparsing as pp
import sys
from jinja2 import Template, Environment

# define grammar
singleLineComment = "#" + pp.restOfLine
identifier = pp.Word( pp.alphas, pp.alphanums + "_").setName("identifier")
attr = pp.Group(identifier + pp.Literal("=>").suppress() + pp.QuotedString(quoteChar="'", escChar="\\"))
entry = pp.Dict(pp.Group(pp.Literal('{').suppress() + pp.delimitedList(attr) + pp.Literal('}').suppress())).setName("entry")
bnf = pp.Literal('[').suppress() + (pp.delimitedList(entry)) + pp.Optional(pp.Literal(',').suppress()) + pp.Literal(']').suppress()
bnf.ignore(singleLineComment)

env = Environment(trim_blocks=True)
tmpl = env.from_string(open(sys.argv[1], "r").read())

entries = [ dict([(x[0], x[1]) for x in entry]) for entry in bnf.parseFile( sys.argv[2] )]
entries = dict([(x["typname"], x) for x in entries])
print (tmpl.render(src=sys.argv[2], entries=entries))
