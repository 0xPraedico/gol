### Rapport — Mini-projet “Jeu de la vie” et benchmark

### Objectif
Implémenter le **Jeu de la vie** (grille 2D vivante/morte, voisins 8-connectés) avec :
- **calcul de la génération suivante sans mise à jour in-place** (double buffer),
- **lecture/écriture fichier** (grille initiale + sauvegarde après \(N\) itérations),
- **UI SDL2** (affichage couleurs + contrôles clavier),
- **historique back/forward**,
- et une **comparaison de performances** entre 2 structures de données d’historique.

La racine contient 2 projets :
- `projet-listechainee/` : historique via **liste doublement chaînée** (baseline)
- `projet-ringbuffer/` : historique via **ring buffer** (buffer circulaire borné, optimisé)
- `bench/bench.sh` : **benchmark comparatif** reproductible

### Architecture (commune aux deux projets)
Chaque projet suit une architecture modulaire en compilation séparée :
- **`Grid`** : représentation compacte en 1D contigu (`uint8_t* cells`) avec mapping \((x,y) \rightarrow y*w + x\), allocation dynamique.
- **`life_step(cur, next)`** : noyau de calcul Conway. La génération suivante est produite dans `next`, puis “commit” via un snapshot dans l’historique.
- **`io`** : chargement/sauvegarde au format texte :
  - première ligne `width height`
  - puis `height` lignes de `.` / `O` (espaces tolérés en fin de ligne)
- **`ui_sdl`** : rendu de la grille en SDL2 (cellules vivantes colorées, mortes sombres), boucle d’événements et mapping des touches.
- **`main`** :
  - mode UI (SDL2) : play/pause, step, back/forward, save, resize…
  - mode batch (headless) : `--input` + `--steps` + `--output`

### Choix de structure de données — Pourquoi un ring buffer pour l’historique ?
Dans `projet-ringbuffer/`, l’historique est une **timeline bornée** implémentée par un **buffer circulaire de pointeurs** vers des snapshots (`Grid*`).

#### Problème à résoudre
L’usage “historique” est **séquentiel** :
- accès à la génération courante, précédente, suivante,
- ajout de la génération suivante au fur et à mesure,
- possibilité de “revenir dans le passé” puis de refaire un step → il faut **tronquer le futur**.

#### Avantages du ring buffer (vs liste chaînée)
- **O(1) strict** pour `back`, `forward`, `current`.
- **push O(1) strict** côté conteneur : pas de `malloc`/`free` de nœuds à chaque step, pas de `realloc` de structure (capacité fixée).
- **Mémoire bornée** : au plus `cap` snapshots stockés → comportement stable sur longues exécutions.
- **Meilleure localité cache** : tableau contigu de pointeurs, moins de “pointer chasing” qu’une liste.
- **Éjection simple** quand plein : libération du plus ancien snapshot et avance de `start` (index circulaire).

#### Règles de fonctionnement (résumé)
- `cap` : capacité max en snapshots (`--history-cap`)
- `start` : index physique du plus ancien
- `len` : nombre d’éléments (0..cap)
- `cur` : position relative (0 = plus ancien, `len-1` = plus récent)
- mapping : `pos_phys(rel) = (start + rel) % cap`

Cas lors d’un `push` :
- si on est au milieu (`cur != len-1`) → **tronquer le futur** (free snapshots `[cur+1..len-1]`, `len=cur+1`)
- si place dispo (`len < cap`) → écrire à la fin logique, `len++`, `cur=len-1`
- si plein (`len==cap`) → **éjecter le plus ancien**, `start=(start+1)%cap`, écrire en fin logique, `cur=len-1`

### Benchmarking : principe, équité, lecture des résultats
#### Objectif du bench
Mesurer objectivement l’impact de la structure d’historique (liste vs ring) sur :
- **temps total** pour \(S\) steps,
- **débit** (steps/s),
- **temps moyen** (ns/step),
à paramètres identiques : `--width --height --steps --seed --history-cap`.

#### Méthode
Chaque projet a une cible :
- `make bench` → binaire `bin/life_bench` (sans SDL), compilé en **-O3** et `-DNDEBUG`.

Le bench :
- initialise une grille pseudo-aléatoire déterministe via `--seed`,
- boucle `S` fois : `life_step` + **push dans l’historique** (pour simuler l’usage réel),
- simule une petite navigation `back/forward` périodique,
- mesure le temps via `clock_gettime(CLOCK_MONOTONIC, ...)`.

#### Script de comparaison
`bench/bench.sh` :
- build les 2 bins bench,
- exécute les deux avec **exactement les mêmes arguments**,
- affiche un résumé et le gagnant (sur `steps/s`).

Commande type (reproductible) :

```bash
./bench/bench.sh --width 512 --height 512 --steps 2000 --seed 42 --history-cap 512
```

#### Interprétation
- **Plus grand `steps/s` = mieux**.
- **Plus petit `ns/step` = mieux**.
- `history-cap` influence fortement :
  - liste : plus de nœuds/snapshots → plus de gestion mémoire et pointeurs,
  - ring : coût structurel stable, mémoire bornée, éjections maîtrisées.

### Commandes “fichier → N itérations → fichier” (mode batch, sans SDL)
Le programme lit la grille initiale depuis `--input`, calcule `--steps N` générations, puis écrit la grille finale dans `--output`.

#### Liste chaînée

```bash
./projet-listechainee/bin/life --input projet-listechainee/data/glider.txt --steps 200 --output out_list.txt --history-cap 512
```

#### Ring buffer

```bash
./projet-ringbuffer/bin/life --input projet-ringbuffer/data/glider.txt --steps 200 --output out_ring.txt --history-cap 512
```
