cd /d "%~dp0"
start /wait cmd /k "call build_core.bat && exit"
start /wait cmd /k "call build_mq.bat && exit"
start /wait cmd /k "call build_blender.bat && exit"
start /wait cmd /k "call build_maya.bat && exit"
start /wait cmd /k "call build_max.bat && exit"
start /wait cmd /k "call build_motionbuilder.bat && exit"
start /wait cmd /k "call build_modo.bat && exit"
