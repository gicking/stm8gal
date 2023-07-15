Import("env")

env.Replace(PROGNAME="%s" % env.GetProjectOption("prog_name"))