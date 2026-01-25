<?php
if (isset($_REQUEST['sleep'])) {
		sleep((int)$_REQUEST['sleep']);
		exit;
}
if (!isset($_REQUEST['gw'])) {
		// Ligne à modifier en indiquant l'adresse de votre passerelle (et port) 
		$gw = 'wss://mapasserelleminipavi.com:YYYY/';
		//////////////////////////////////////////////////////////
} else {
		$gw = $_REQUEST['gw'];
}

if (!isset($_REQUEST['url'])) {
		$url = $gw;
} else {
		$url = $gw.'?url='.urlencode($_REQUEST['url']);
}
if (isset($_REQUEST['base'])) 
	$base = true;
else $base = false;
?>

<!DOCTYPE html>
<html>
  <head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0, shrink-to-fit=no">
    <meta http-equiv="X-UA-Compatible" content="IE=edge">
	<meta http-equiv="Access-Control-Allow-Origin" content="*">
    <title>-= Emulateur Minitel =-</title>
    <link rel="stylesheet" href="css/minitel-real-viewer.css" />
	<link rel="stylesheet" href="css/minitel-minipavi-webmedia.css" />
	<link rel="stylesheet" href="css/crt.css" /> 
    <meta name="keywords" content="MiniPavi, minitel, vintage, années 80, technologie, videotex, stum1b, teletel, 3615" />
    <meta name="description" content="Créez votre services Minitel facilement! Voyagez dans le passé et aidez à la sauvergarde du patrimoine technologique !" />
	<meta name="author" content="Jean-arthur Silve" />	
	<meta property="og:type" content="website">
	<meta property="og:site_name" content="MiniPAVI">
	<meta property="og:description" content="Créez votre services Minitel facilement! Voyagez dans le passé et aidez à la sauvergarde du patrimoine technologique !">
    <meta property="og:title" content="Accueil service MiniPAVI">
	<meta property="og:url" content="http://www.minipavi.fr">
  </head>
  <body>
    <div id="minitel-viewer">
	<br/>
	<x-minitel id="emul-1" data-socket="<?php echo $url;?>"
                 data-speed="1200"
                 data-color="true">
			<div class="minitel-wrapper"> 
			<canvas class="minitel-screen" data-minitel="screen"></canvas>
			<canvas class="minitel-cursor" data-minitel="cursor"></canvas>
		</div>
			
        <audio class="minitel-beep" data-minitel="beep">
          <source src="sound/minitel-bip.mp3" type="audio/mpeg"/>
          Too bad, your browser does not support HTML5 audio or mp3 format.
        </audio>      
		<div id="minitel-glass"></div>
		<div  style="<?php if ($base) { ?>display:none;<?php }?>position:absolute;top:0;margin:auto;background-color:black;width:25%;border-radius:8px;padding:10px;margin-left:10px;margin-top:10px;">
		<import src="import/minitel-keyboard.html"></import>
		<import src="import/minitel-minipavi-webmedia.html"></import>	
		</div>
		
      </x-minitel>
	  
	 </div>
	  
	  
	  
	  
		<?php
		if (!$base) {
		?>
		<div style="grid-row: 2;">
		<br/>

		<div style="max-width:1000px; margin:40px auto; padding:20px; border:2px solid #da33e2; text-align:center; font-family:Arial, sans-serif; background-color:white; box-shadow:inset 0 0 0 1px #d3d3d3; border-radius:8px;">		
			<code><?php echo $url;?></code><br/><br/>
			<?php if (!isset($_REQUEST['gw'])) { ?>
				Après la connexion à un service, "Cnx/fin" pour revenir à la page d'accueil MiniPavi.
				<br/>
				(sauf en cas d'accès direct à un service)
				<br/><br/>
			<?php }?>
			Plus d'infos sur MiniPAVI
			<br/><a href="https://www.minipavi.fr/" style="color:#da33e2;" target="_blank">https://www.minipavi.fr/</a>
			<br/>Emulateur Minitel Frédéric BISSON (version modifiée MiniPavi)
			<br/><a href="https://minitel.cquest.org/" style="color:#da33e2;" target="_blank">https://minitel.cquest.org/</a>
		</div>
		<?php } ?>
	  </div>
    


    <script src="library/generichelper/generichelper.js"></script>
    <script src="library/import-html/import-html.js"></script>
    <script src="library/autocallback/autocallback.js"></script>
    <script src="library/query-parameters/query-parameters.js"></script>
    <script src="library/finite-stack/finite-stack.js"></script>
    <script src="library/key-simulator/key-simulator.js"></script>
    <script src="library/settings-suite/settings-suite.js"></script>
    <script src="library/minitel/constant.js"></script>
    <script src="library/minitel/protocol.js"></script>
    <script src="library/minitel/elements.js"></script>
    <script src="library/minitel/text-grid.js"></script>
    <script src="library/minitel/char-size.js"></script>
    <script src="library/minitel/font-sprite.js"></script>
    <script src="library/minitel/page-cell.js"></script>
    <script src="library/minitel/vram.js"></script>
    <script src="library/minitel/vdu-cursor.js"></script>
    <script src="library/minitel/vdu.js"></script>
    <script src="library/minitel/decoder.js"></script>
    <script src="library/minitel/keyboard.js"></script>
    <script src="library/minitel/minitel-emulator.js"></script>
    <script src="library/minitel/start-emulators.js"></script>
	<script src="library/minitel/minitel-minipavi-webmedia.js"></script>
	
    <script src="app/minitel.js"></script>
	<?php
	if ($base) {
	?>
	<img src="indexminitel.php?sleep=2" style="width:0px; height:0px;" />
	<?php } ?>


  </body>
</html>
