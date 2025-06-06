# Editeur de texte avec Harvey

## REMARQUE :

Le projet est fonctionnel mais avec une série d'erreur qui surviennent de manière aléatoires. Si une erreur interrompt le code, relancer et taper *lentement*.

[Voir la section sur les erreurs](#erreurs)

## Le projet

Ce projet consiste en la création d'un petit éditeur de texte. Mon premier objectif était d'implémenter les fonctions les plus basiques telles que :

- écrire un caractère
- effacer un caractère
- tabulation
- espace
- retour à la ligne
- curseur visible
- défilement

Pour l'utiliser, rien de plus simple : utilisez votre clavier comme vous le feriez sur n'importe quelle machine pour taper du texte.

## Installation et utilisation

### Installation 

- extraire le dossier source 

ou 

- executer la commande : 

```sh

git clone https://github.com/Pallandos/txteditor.git

```

### Utilisation

Dans le répertoire du dossier source, executer : 

```sh

make exec

```

## La réalisation

Je me suis basé sur le dépôt fourni dans le cadre du cours et je n'ai modifié que le `main.c`.

Mon idée était d'utiliser les interruptions du clavier pour récuperer les codes ASCII, les mettre dans une *queue* et vider celle ci en affichant à l'écran. 

### `keyboard_interrupt_handler()`

Premier composant central de mon programme : la routine d'interruption du clavier. Celle ci va récupérer le code de la touce pressée : 

```.c

kdata = KEYBOARD->DATA

```

Une fois `kdata` enregistrée on la met dans une *queue* (FIFO) qui sera vidée par la *task* qui affiche à l'écran.

> Le `xprintf` ne sert que à des fins de débuguage.

### L'impression des caractères

La tâche `write_task(void *arg)` s'occupe d'afficher les caractères à l'écran. Une fois lancée, elle va afficher les caractères à l'écran aux positions `(x,y)`, et ces positions sont actualisées au cours de l'execution.

La structure de la tâche est la suivante : 

```c

void write_task(void *arg)
{
	(void)arg;

	int text_x_pos = 0;
	int text_y_pos = 0;

    uint32_t kdata;

	while (1) {

		xSemaphoreTake(video_refresh_semaphore, portMAX_DELAY);
		font_8x16_draw_char(text_x_pos, text_y_pos, ' ', BLACK, WHITE);

		while (xQueueReceive(kb_queue, &kdata, 0) == pdTRUE) {
			
			xSemaphoreTake(video_refresh_semaphore, portMAX_DELAY);

            // gestion des caractères spéciaux

			char c = (char)KEYBOARD_KEY_CODE(kdata);
			font_8x16_draw_char(text_x_pos, text_y_pos, c, WHITE, BLACK);

			// move the cursor
			text_x_pos += 10;
		}
	}
}

```

Une boucle infinie s'occupe de vider la *queue*, attends que le sémaphore de raffraichissement soit disponible, puis s'occupe du caractère pressé. On va d'abord évaluer les cas des caractères spéciaux (espace, touche **DEL**, tabulation, entrer, flèche haut et flèche bas) et les traiter dans leur cas spécial. Par exemple la touche *entrer* : 

```c

// if return : go to next line
if (KEYBOARD_KEY_CODE(kdata) == 13) {
	font_8x16_draw_char(text_x_pos, text_y_pos, ' ', WHITE, BLACK);
	text_x_pos = 0;
	if (text_y_pos >= Y_MAX - 16) {
		text_y_pos = 0;
	} else {
		text_y_pos += 16;
	}
	continue;
}

```

Si ce n'est pas un caractère spécial, on convertit le code ASCII en `char` et on l'affiche sur l'écran avec la fonction `font_8x16_draw_char`.

> Le programme déplace auss un curseur blanc afin de se repérer sur l'écran.

## Erreurs

Bien que mon code soit fonctionnel, il survient de temps en temps cette erreur : `Instruction access fault exception @ PC: 0x00000000`. 

Pourtant, je ne parviens pas à identifier la cause de cette erreur car elle survient à des moments "aléatoires", sans aucune corrélation entre eux, que ce soit en temps ou en nombre de touches pressées. 

Je suppose qu'il s'agit d'une erreur liée à une surcharge des capacités de ma machine, due à une utilisation peu optimale des ressources. En effet la tâche qui affiche les caractères est implémentée en attente active. J'aurais aimé pouvoir endormir la tâche quand la *queue* est vide et ne la réveiller que quand on la remplie, mais j'ai manqué de temps pour cela. 

## Conclusion 

Ce petit projet m'a permis de réaliser la complexité de concevoir même les programmes les plus simples lorsque l'on travaille à bas niveau. 

J'ai bien réussi à créer un éditeur de texte, rudimentaire mais fonctionnel. 

Les pistes d'améliorations seraient :

- amélioration de l'efficacité avec une attente passive
- prise en charge du défilement
- prise en charge des combinaisons de touches
