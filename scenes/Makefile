all : \
    ../dist/board.pnct \
    ../dist/board.scene \

../dist/board.scene : board.blend export-scene.py
    "C:\Program Files\Blender Foundation\Blender 2.90\blender.exe" --background --python export-scene.py -- "board.blend:Collection" "../dist/board.scene"

../dist/board.pnct : board.blend export-meshes.py
    "C:\Program Files\Blender Foundation\Blender 2.90\blender.exe" --background --python export-meshes.py -- "board.blend:Collection" "../dist/board.pnct" 