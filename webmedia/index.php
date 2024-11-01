<!DOCTYPE html>
<html lang="fr">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <meta http-equiv="X-UA-Compatible" content="IE=edge" />
	<meta property="og:type" content="website" />
	<meta property="og:title" content="MiniPavi, Minitel is still alive !"/>
	<meta property="og:description" content="Accèdez à des service Minitels et créez le vôtre facilement ! Voyagez dans le passé ! Aidez à la conservation du patrimoine technologique !"/>
	<meta property="og:locale" content="fr_FR" /> 	  
	<meta name="keywords" content="MiniPavi, minitel, télématique, transpac, vintage, années 80, technologie, videotex, stum1b, teletel, 3615" />
	<meta name="description" content="Accèdez à des service Minitels et créez le vôtre facilement ! Voyagez dans le passé !" />
	<meta name="author" content="Jean-Arthur SILVE" />
	<link rel="stylesheet" href="webmedia.css" />
	
    <title>Accès MiniPavi WebMedia</title>
</head>
<body>
	<script>
		var tag = document.createElement('script');
		tag.src = "https://www.youtube.com/iframe_api";
		var firstScriptTag = document.getElementsByTagName('script')[0];
		firstScriptTag.parentNode.insertBefore(tag, firstScriptTag);
	</script>
	<script src="webmedia.js"></script>

    
    <div id="content">
		<img src="images/logo.png" style="max-width:300px;"/>
		<h1>Accès WebMedia</h1>
		<p id="instructions" style="display:none;">Saisissez le code Pin de votre connexion à MiniPavi pour accèder à son contenu multimedia.
		<br/><span style="font-size:11px">Le code pin est indiqué en haut à gauche de l'écran d'accueil MiniPavi sur votre Minitel.</span></p>
		
		<form id="pinForm" style="display:none;">
			
			<input type="text" id="pin1" maxlength="1" class="pin-input" inputmode="numeric" required>
			<input type="text" id="pin2" maxlength="1" class="pin-input" inputmode="numeric" required>
			<input type="text" id="pin3" maxlength="1" class="pin-input" inputmode="numeric" required>
			<input type="text" id="pin4" maxlength="1" class="pin-input" inputmode="numeric" required>
			
			<input type="hidden" id="pin" name="pin">
		</form>
		<button class="submit-button" id="validate-btn" onclick='validateForm();'>Valider</button>
		<p id="error" style="display:none;">Code Pin invalide.</p>
		<div id="loader" class="loader"></div>
		<div id="contentMessage"></div>
		<audio id="audioPlayer" controls autoplay></audio>
		<div id="youtubePlayer"></div>
		<video id="videoPlayer" controls playsinline autoplay allow="fullscreen" style="max-width: 100%;height: auto;display:block;"></video>			
		<img id="imgViewer"></img>
		<button id="linkButton" data-filename=""></button>
	
	
		<script>
		const urlParams = new URLSearchParams(window.location.search);
		const pin = urlParams.get('pin');


		if (pin && /^\d{4}$/.test(pin)) {
			const sanitizedPin = pin.replace(/</g, "&lt;").replace(/>/g, "&gt;");
			document.getElementById('instructions').style.display = 'block';
			document.getElementById('pinForm').style.display = 'flex';
			document.getElementById("pin1").focus();
			document.addEventListener('DOMContentLoaded', function() {
			prefillPin(pin);
			document.getElementById('validate-btn').style.display = 'flex';
			});
			
		} else {
		    document.getElementById('instructions').style.display = 'block';
            document.getElementById('pinForm').style.display = 'flex';
			document.getElementById("pin1").focus();
		}
		</script>
	
		<br/><br/>
		Plus d'infos sur MiniPAVI - <a href="https://www.minipavi.fr/" style="color:white;" target="_blank">https://www.minipavi.fr/</a>	
		
    </div>
</body>
</html>
