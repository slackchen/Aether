import os
import sys
import subprocess

base = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
upstream = os.path.join(base, 'emsdk', 'upstream', 'install')
emscripten_root = os.path.join(upstream, 'emscripten')
emcc_py = os.path.join(emscripten_root, 'emcc.py')

env = os.environ.copy()
env['PATH'] = emscripten_root + os.pathsep + os.path.join(upstream, 'bin') + os.pathsep + env.get('PATH', '')

sources = [
    'src/main.cpp',
    'src/webgpu/webgpu_device.cpp',
    'src/webgpu/webgpu_swapchain.cpp',
    'src/webgpu/webgpu_buffer.cpp',
    'src/webgpu/webgpu_shader.cpp',
    'src/webgpu/webgpu_pipeline.cpp',
    'src/engine/renderer.cpp',
]

src_paths = [os.path.join(base, s) for s in sources]

output_dir = os.path.join(base, 'build')
os.makedirs(output_dir, exist_ok=True)

output_html = os.path.join(output_dir, 'aether.html')

args = [
    'python', emcc_py,
    '-std=c++20',
    '-O3',
    '-Wall',
    '-Wextra',
    '--use-port=emdawnwebgpu',
    '-s', 'WASM=1',
    '-s', 'ALLOW_MEMORY_GROWTH=1',
    '-s', 'NO_EXIT_RUNTIME=1',
    '-s', "EXPORTED_RUNTIME_METHODS=['ccall','cwrap']",
    '--shell-file', os.path.join(base, 'web', 'shell.html'),
    '-I', os.path.join(base, 'src'),
    '-o', output_html,
] + src_paths

print('Compiling...')
print('Command:', ' '.join(args))
result = subprocess.run(args, env=env, cwd=base)

if result.returncode != 0:
    print(f'Build failed with code {result.returncode}')
    sys.exit(1)

print('Build successful!')
print(f'Output: {output_html}')
