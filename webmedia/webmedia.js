/**
 * @file webmedia.js
 * @author Jean-arthur SILVE <contact@minipavi.fr>
 * @version 1.1
 */

var YTPlayer;
let intervalId = null;
let isFetching = false; 
let hasContentBeen1 = false;
let instructionsRemoved = false;
let lastEvent = '';
let YTready = false;

function callMiniPaviWM(pinValue) {
	
	
	// Ligne à modifier en indiquant l'adresse de votre passerelle (et port) 
	
	const url = 'https://mapasserelleminipavi.com:XXXX?action=webmedia';	
   
	///////////////////////////////////////////////////////////////////////

   
	intervalId = setInterval(() => {
		if (!isFetching) {
			isFetching = true; // Active le verrou
			const fullUrl = `${url}&pin=${encodeURIComponent(pinValue)}&lastevent=${lastEvent}`;
			lastEvent = '';
			fetch(fullUrl)
				.then(response => {
					showLoader(false);
					if (!response.ok) {
						throw new Error('Erreur réseau : ' + response.statusText);
					}
					return response.json();
				})
				.then(data => {
					showLoader(false);
					console.log('Réponse JSON:', data);
					handleApiResponse(data);  // Appel de la fonction pour gérer la réponse
				})
				.catch(error => {
					showLoader(false);
					console.error('Erreur:', error);
				})
				.finally(() => {
					isFetching = false; // Libère le verrou une fois la réponse reçue
				});
		}
	}, 1000); 
}			
	

function handleApiResponse(data) {
	const pinForm = document.getElementById('pinForm');
	const errorElement = document.getElementById('error');
	const contentMessage = document.getElementById('contentMessage');
	const instructions = document.getElementById('instructions');
	const audioPlayer = document.getElementById('audioPlayer');
	const youtubePlayer = document.getElementById('youtubePlayer');
	const videoPlayer = document.getElementById('videoPlayer');
	const imgViewer = document.getElementById('imgViewer');
	const linkButton = document.getElementById('linkButton');
	const validateBtn = document.getElementById('validate-btn');
	
	
	validateBtn.style.display='none';
	if (data.result === 'KO') {
		// Réafficher le formulaire avec un message d'erreur
		errorElement.style.display = 'block';
		pinForm.style.display = 'flex';
		contentMessage.style.display = 'none';
		resetForm();
		clearInterval(intervalId); // Arrêter l'appel à l'URL
		stopYoutubePlayer();  // Arrêter le lecteur YouTube
		stopAudioPlayer();  // Arrêter le lecteur audio
		stopVideoPlayer();  // Arrêter le lecteur video
		stopImgViewer();						
		stopDownloadButton();
	} else if (data.result === 'OK') {
		errorElement.style.display = 'none';
		pinForm.style.display = 'none';

		if (!instructionsRemoved) {
			instructions.style.display = 'none';
			instructionsRemoved = true;
		}

		if (data.content === '1') {
			hasContentBeen1 = true; // Marquer que content a été 1
			contentMessage.innerHTML = '';
			if (data.type === 'IMG') {
				stopYoutubePlayer();
				stopVideoPlayer();
				stopAudioPlayer();
				stopDownloadButton();
				imgViewer.src = data.infos;
				imgViewer.style.display = 'block';
			} else if (data.type === 'SND') {
				stopYoutubePlayer();
				stopVideoPlayer();
				stopImgViewer();	
				stopDownloadButton();						
				audioPlayer.src = data.infos;
				audioPlayer.style.display = 'block';
				audioPlayer.onended = function(){eventStopped();};
				audioPlayer.onplaying = function(){eventPlaying();};
				audioPlayer.play();
			} else if (data.type === 'URL') {
				stopYoutubePlayer();  
				stopVideoPlayer(); 
				stopImgViewer();	
				stopAudioPlayer();
				linkButton.setAttribute('data-filename', data.infos); 
				linkButton.innerHTML = "Cliquez pour aller vers<br/><b>"+data.infos+"</b>";
				linkButton.style.display = 'inline-block';
			} else if (data.type === 'YT') {
				if (YTready) {
					YTPlayer.loadVideoById(data.infos);
					YTPlayer.playVideo();
				} else {
					YTready = false;
					stopAudioPlayer();
					stopVideoPlayer();
					stopImgViewer();				
					stopDownloadButton();						
					const youtubeUrl = `https://www.youtube.com/embed/${data.infos}?enablejsapi=1&rel=0`;
					youtubePlayer.innerHTML = `<iframe id="YTPlayer" class="responsive-iframe" src="${youtubeUrl}" title="YouTube video player" frameborder="0" allow="accelerometer; autoplay; clipboard-write; encrypted-media; gyroscope; picture-in-picture; web-share" referrerpolicy="strict-origin-when-cross-origin" allowfullscreen></iframe>`;
					youtubePlayer.style.display = 'block';
					YTPlayer = new YT.Player('YTPlayer', {
					events: {
						'onReady': onPlayerReady,
						'onStateChange': onStateChange,
					}
					});
				}

			} else if (data.type === 'VID') {
				stopYoutubePlayer();  // Arrêter le lecteur YouTube
				stopAudioPlayer();  // Arrêter le lecteur audio
				stopImgViewer();		
				stopDownloadButton();
				videoPlayer.src = data.infos;
				videoPlayer.style.display = 'block';
				videoPlayer.onended = function(){eventStopped();};
				videoPlayer.onplaying = function(){eventPlaying();};
				videoPlayer.play();
			}
		} else if (data.content === '0' && !hasContentBeen1) {
			//contentMessage.innerText = 'Les contenus multimedia du service s\'afficheront ici.';
			contentMessage.innerHTML = 'Les contenus multimedia du service s\'afficheront ici.<br/><img src="../images/pngegg.png" style="max-width: 100%;height: auto;display:block;"/>';
			hasContentBeen1=true;
		}

		contentMessage.style.display = 'block';
	}
}

function eventStopped() {
	lastEvent = 'STOP';
	console.log("Arret lecteur");
}

function eventPlaying() {
	lastEvent = 'START';
	console.log("Lecture");
}


function onPlayerReady(event) {
	console.log('YT READY');
	YTready = true;
	event.target.playVideo();
}

function onStateChange(event) {
	if (event.data == 0)
		eventStopped();
	else if (event.data == 1)
		eventPlaying();
}


function showLoader(show) {
	const loaderAnim = document.getElementById('loader');
	if (show)
		loaderAnim.style.display = 'grid';
	else loaderAnim.style.display = 'none';
}

function stopAudioPlayer() {
	const audioPlayer = document.getElementById('audioPlayer');
	audioPlayer.pause();
	audioPlayer.currentTime = 0; // Remettre à zéro le lecteur audio
	audioPlayer.style.display = 'none';
}

function stopImgViewer() {
	const imgViewer = document.getElementById('imgViewer');
	imgViewer.style.display = 'none';
}

function stopVideoPlayer() {
	const videoPlayer = document.getElementById('videoPlayer');
	videoPlayer.pause();
	videoPlayer.currentTime = 0; // Remettre à zéro le lecteur video
	videoPlayer.style.display = 'none';
}

function stopYoutubePlayer() {
	const youtubePlayer = document.getElementById('youtubePlayer');
	youtubePlayer.innerHTML = ''; // Supprimer l'iframe du lecteur YouTube pour arrêter la lecture
	youtubePlayer.style.display = 'none'; // Masquer le lecteur YouTube si visible
	YTready = false;
}

function stopDownloadButton() {
	const linkButton = document.getElementById('linkButton');
	linkButton.style.display = 'none';
}



function prefillPin(pin) {
	var pinInputs = document.querySelectorAll('.pin-input');
	for (let i = 0; i < 4; i++) {
		pinInputs[i].value = pin[i] || '';
	}
}

function validateForm() {
	const inputs = document.querySelectorAll('.pin-input');
	const pinValue = Array.from(inputs).map(input => input.value).join('');
	const validateBtn = document.getElementById('validate-btn');
	validateBtn.style.display='none';
	document.getElementById('pin').value = pinValue;
	showLoader(true);
	document.getElementById('error').style.display = 'none';
	callMiniPaviWM(pinValue);
}


function resetForm() {
	const inputs = document.querySelectorAll('.pin-input');
	inputs.forEach(input => input.value = '');
	inputs[0].focus(); // Remet le focus sur la première case
}

document.addEventListener('DOMContentLoaded', function() {
	const audioPlayer = document.getElementById('audioPlayer');
	const youtubePlayer = document.getElementById('youtubePlayer');
	const videoPlayer = document.getElementById('videoPlayer');
	const imgViewer = document.getElementById('imgViewer');
	const linkButton = document.getElementById('linkButton');
	const validateBtn = document.getElementById('validate-btn');
	const inputs = document.querySelectorAll('.pin-input');

	videoPlayer.style.display = 'none';
	youtubePlayer.style.display = 'none';
	audioPlayer.style.display = 'none';
	linkButton.style.display = 'none';
	imgViewer.style.display = 'none';


	linkButton.addEventListener('click', function() {
		
		const fileUrl = this.getAttribute('data-filename');
		const a = document.createElement('a'); 
		a.href = fileUrl;
		a.target = '_blank';
		document.body.appendChild(a);
		a.click();
		document.body.removeChild(a);
	});


	inputs[0].focus(); // Met le focus sur la première case au chargement de la page

	inputs.forEach((input, index) => {
		input.addEventListener('input', function() {
			if (/\D/.test(this.value)) {
				this.value = ''; // Supprime les caractères non numériques
				return;
			}
			validateBtn.style.display='none';
			if (this.value.length === this.maxLength) {
				const nextInput = inputs[index + 1];
				if (nextInput) {
					
					nextInput.focus();
				} else {
					validateForm();
				}
			}
		});

		input.addEventListener('keydown', function(event) {
			if (event.key === "Backspace" && input.value === '') {
				const prevInput = inputs[index - 1];
				if (prevInput) {
					prevInput.focus();
				}
			}
		});
	});
});
