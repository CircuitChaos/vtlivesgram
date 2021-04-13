env = Environment()
env['CCFLAGS']	= '-Wall -Wextra -O2 -Wno-psabi'
env['CPPPATH']	= 'src'
env['LIBS'] = ['m', 'fftw3', 'X11']

env.VariantDir('build', 'src', duplicate = 0)
vtlivesgram = env.Program('build/vtlivesgram', Glob('build/*.cpp'))

env.Install('/usr/local/bin', vtlivesgram)
env.Alias('install', '/usr/local/bin')
