from mc import *
def a(e):
	print(e)
	sendForm(UUID('twoone3'), '{"content":[{"type":"label","text":"这是一个文本标签"},{"placeholder":"水印文本","default":"","type":"input","text":""},{"default":true,"type":"toggle","text":"开关~或许是吧"},{"min":0.0,"max":10.0,"step":2.0,"default":3.0,"type":"slider","text":"游标滑块！？"},{"default":1,"steps":["Step 1","Step 2","Step 3"],"type":"step_slider","text":"矩阵滑块？!"},{"default":1,"options":["Option 1","Option 2","Option 3"],"type":"dropdown","text":"如你所见，下拉框"}], "type":"custom_form","title":"这是一个自定义窗体"}')
	#print(sendForm(UUID('twoone3'),'test'))
	#print(setPlayerScore(UUID('twoone3'),'test',1,1))
	#addItem(UUID('twoone3'),1,0,1)
	#tellraw(UUID('twoone3'),'提示')
	#runcmdAs(UUID('twoone3'),'say 不是吧啊这')
	#teleport(UUID('twoone3'),e['XYZ'][0],e['XYZ'][1],e['XYZ'][2],e['dimensionid'])
	#transferServer(UUID('twoone3'),'49.232.53.205',19134)
	return True
setListener('破坏方块',a)
setListener('选择表单',a)