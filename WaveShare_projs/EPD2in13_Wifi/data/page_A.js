var srcBox, srcImg, dstImg;
var epdArr, epdInd, palArr;
var curPal;

function getElm( n ) { return document.getElementById(n); }
function setInn( n, i ) { document.getElementById(n).innerHTML = i; }
 
function processFiles( files ) {
	var file = files[0];
	var reader = new FileReader();
	srcImg = new Image();
	reader.onload=function( e ) {
		setInn('srcBox', '<img id=\"imgView\" class=\"sourceImage\">');
		var img = getElm('imgView');
		img.src = e.target.result;
		srcImg.src = e.target.result;
	};
	
	reader.readAsDataURL(file);
}
//-------------------------------------------
function drop( e ) {
	e.stopPropagation();
	e.preventDefault();
	var files = e.dataTransfer.files;
	processFiles(files);
}
//-------------------------------------------
function ignoreDrag( e ) {
	e.stopPropagation();
	e.preventDefault();
}
//-------------------------------------------
function getNud( nm, vl ){
	return '<td class=\"comment\">' + nm + ':</td>' +
			'<td><input id=\"nud_' + nm + '\" class=\"nud\"type=\"number\" value=\"' + vl + '\"/></td>';
}
//-------------------------------------------
function Btn( nm, tx, fn ) {
	return '<div><label class=\"menu_button\" for=\"_' + nm + '\">' + tx + '</label>'+
			'<input class=\"hidden_input\" id=\"_' + nm + '\" type=\"' +
			(nm == 0 ? 'file\" onchange=\"' : 'button\" onclick=\"') + fn + '\"/></div>';
}
//-------------------------------------------
function RB(vl,tx) {
	return '<input type=\"radio\" name=\"kind\" value=\"m'+vl+
			'\" onclick=\"rbClick('+vl+');\"'+(vl==0?'checked=\"true\"':'')+'/>'+tx;
}
//-------------------------------------------
window.onload = function() {
	srcBox = getElm('srcBox');
	srcBox.ondragenter=ignoreDrag;
	srcBox.ondragover=ignoreDrag;
	srcBox.ondrop=drop;
	srcImg = 0;
	epdInd = 0;
 
	palArr = [
		[[0,0,0], [255,255,255]],
		[[0,0,0], [255,255,255], [127,0,0]],
		[[0,0,0], [255,255,255], [127,127,127]],
		[[0,0,0], [255,255,255], [127,127,127], [127,0,0]],
		[[0,0,0], [255,255,255]],
		[[0,0,0], [255,255,255], [220,180,0]],
		[[0,0,0]],
		[[0,0,0], [255,255,255], [0,255,0], [0,0,255], [255,0,0], [255,255,0], [255,128,0]]
	];	//5.65f 7-color E-Paper 
	
	epdArr = [
		[200,200,0], [200,200,3], [152,152,5],
		[122,250,0], [104,212,1], [104,212,5], [104,212,0],
		[176,264,0], [176,264,1],
		[128,296,0], [128,296,1], [128,296,5], [128,296,0],
		[400,300,0], [400,300,1], [400,300,5],
		[600,448,0], [600,448,1], [600,448,5],
		[640,384,0], [640,384,1], [640,384,5],
		[800,480,0], [800,480,1], [880,528,1],
		[600,448,7], [880,528,0], [280,480,0],
		[152,296,0], [648,480,1], [128,296,1],
		[200,200,1], [104,214,1], [128,296,0],
		[400,300,1], [152,296,1], [648,480,0],
		[640,400,7], [176,264,1], [122,250,0],
		[122,250,1], [240,360,0], [176,264,0],
		[122,250,0], [400,300,0], [960,680,0],
		[800,480,0], [128,296,1], [960,680,1]
	];
 
	setInn('BT',
		Btn(0, 'Select image file', 'processFiles(this.files);') +
		Btn(1, 'Level: mono', 'procImg(true,false);') +
		Btn(2, 'Level: color', 'procImg(true,true);') +
		Btn(3, 'Dithering: mono', 'procImg(false,false);') +
		Btn(4, 'Dithering: color', 'procImg(false,true);') +
		Btn(5, 'Upload image', 'uploadImage();')
	);
	
	setInn('XY', getNud('x', '0') + getNud('y', '0'));
	setInn('WH', getNud('w', '200') + getNud('h', '200'));
	
	setInn('RB', RB(0, '1.54&ensp;') + RB(1, '1.54b') + RB(2, '1.54c&ensp;<br>') +
		RB(3, '2.13&ensp;') + RB(4, '2.13b') + RB(5, '2.13c<br>') +
		RB(6, '2.13d') + RB(7, '2.7&ensp;&ensp;') + RB(8, '2.7b&ensp;<br>') +
		RB(9, '2.9&ensp;&ensp;') + RB(10, '2.9b&ensp;') + RB(11, '2.9c&ensp;<br>') +
		RB(12, '2.9d&ensp;') + RB(13, '4.2&ensp;&ensp;') + RB(14, '4.2b&ensp;<br>') +
		RB(15, '4.2c&ensp;') + RB(16, '5.83&ensp;') + RB(17, '5.83b<br>') +
		RB(18, '5.83c&ensp;') + RB(19, '7.5&ensp;&ensp;') + RB(20, '7.5b&ensp;<br>') +
		RB(21, '7.5c') + RB(22, '7.5 V2') + RB(23, '7.5b V2<br>') +
		RB(24, '7.5b HD&ensp;') + RB(25, '5.65f<br>') +
		RB(26, '7.5 HD&ensp;') + RB(27, '3.7&ensp;') + RB(28, '2.66<br>')+
		RB(29, '5.83b V2&ensp;') + RB(30, '2.9b V3<br>') +
		RB(31, '1.54b V2&ensp;') + RB(32, '2.13b V3<br>') +
		RB(33, '2.9 V2&ensp;') + RB(34, '4.2b V2<br>') +
		RB(35, '2.66b&ensp;') + RB(36, '5.83 V2<br>') +
		RB(37, '4.01 f&ensp;')+ RB(38, '2.7b V2<br>') +
		RB(39, '2.13 V3&ensp;')+ RB(40, '2.13 B V4<br>') +
		RB(41, '3.52&ensp;') + RB(42, '2.7 V2<br>') +
		RB(43, '2.13 V4&ensp;') + RB(44, '4.2 V2<br>') +
		RB(45, '13.3k&ensp;') + RB(46, '4.26<br>') +
		RB(47, '2.9bV4&ensp;') + RB(48, '13.3b<br>')
	);

	getElm('ip_addr').innerHTML = "Device IP: " + self_IP;
	console.log("element text: " + getElm('ip_addr').innerText);
}
// -------------------------------------------
function rbClick(index){
	getElm('nud_w').value = "" + epdArr[index][0];
	getElm('nud_h').value = "" + epdArr[index][1];
	epdInd=index;
}