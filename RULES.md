# CrazyJack — The Rules

## What Is This

Blackjack. But every player has a superpower, the dealer rotates, and someone will definitely rewind time to undo your winning hand. First to **15 points** wins. Good luck.

---

## The Basics

- Everyone gets **2 cards**. Get as close to **21** as possible without going over.
- One player is the **Host** (dealer) each round. The Host role rotates after every round.
- Non-Host players take turns: **Hit** (draw a card), **Stand** (stop), or **Use a Skill**.
- After everyone's done, the Host hits against each player one-on-one.
- Then **Battle**: your hand vs the Host's. Higher hand wins. Bust = you lose.
- Aces count as 11, 10, or 1 — whichever keeps you alive.

---

## Scoring

| What happened | Points |
|---|---|
| Win a battle | **+2** |
| Tie a battle | **+1** |
| Blackjack (21 with 2 cards) | **+3** |
| Host Blackjack | **+3 + (players + 2)** — round ends instantly, nobody else plays |
| Quintet (5 cards, still under 21) | **+5** |

Reach **15 points** and the game ends. You win. Everyone else copes.

---

## The Skills

Every round, each player is randomly assigned one of five skills. Yes, your skill changes every round. Yes, this is chaos by design.

### Deliverance (Active) — 3 uses/round
Return a card from your hand to the bottom of the deck. Simple, clean, effective.
- **Passive:** Gain +1 use at the start of your turn. Hoard responsibly.

### Neural Gambit (Active) — 2 uses/round
Pick 2 players. Each reveals a card. The rank difference gets added as a **permanent boost** to whichever card you choose.
- Example: revealed a 3 and a 9? That's a +6 boost to one of them. A 3 becomes a 9. A 9 becomes a 15-value card. There is no cap.
- Cards flip back face-down after the reveal. Only you saw the carnage.

### Fatal Deal (Reactive) — 3 uses/round
When **anyone else** draws a card, you see its rank. Then you can **swap** one of your cards with it. The drawer gets your reject, you get their fresh pull.
- Triggers automatically on every opponent draw. You choose whether to strike.
- **Passive:** You always see what was drawn, even if you don't swap.

### ChronoSphere (Fusion) — 3 uses/round
Take a **Snapshot** of your current hand (free). Later, **Rewind** to restore it (costs 1 use). The deck coughs up matching cards and your current hand goes back.
- Snapshots are free and unlimited. Rewinds cost uses.
- **Reactive:** If your hand value changes (e.g., someone Neural Gambits your card), you get a prompt to Rewind immediately.
- **Reactive:** At the start of any player's hit phase, you can proactively Snapshot.

### Destiny Deflect (Reactive) — 3 uses/round
When **anyone** draws a card, intercept it and **redirect** it to a different player. The drawer gets nothing. The target gets a surprise.
- **Passive — Clairvoyance:** At the start of the round, peek at the top 3 cards of the deck. Plan your interceptions.

---

## Reactive Skills — How They Work

Some skills trigger when something happens (a card draw, a phase start). When triggered:
1. You get a **Yes/No prompt** with a countdown timer (12 seconds default).
2. Say Yes → pick your target/card → skill fires.
3. Say No or run out of time → nothing happens.
4. If multiple players have reactive skills, they're prompted **one at a time** in turn order.

---

## Special Events

**Blackjack** — 21 with exactly 2 cards. You get 3 points, skip the rest of the round, and watch everyone else suffer. If the **Host** gets it, the round ends immediately and they get a fat bonus.

**Quintet** — Somehow survive to 5 cards without busting. +5 points. Extremely risky, extremely rewarding.

---

## Multiplayer

- **Local:** You + bots. Classic.
- **Host Online:** You run the server. Friends connect. You are the authority.
- **Join Online:** Enter the host's IP. Wait in the lobby. The host starts when ready.

The host's machine runs all game logic. Clients send actions, receive state. No cheating (in theory).

---

## Bot Behavior

Bots follow simple rules:
- Under 17? Always hit.
- 17-20? Calculate bust risk, factor in opponent pressure, maybe hit.
- 21? Stand (they're not *that* dumb).
- Reactive skills? Bots always accept. Always.

---

## The Chaos, Summarized

You draw a card. Someone with Fatal Deal sees it and swaps it with their garbage. Someone else with Destiny Deflect intercepts your next draw and sends it to the Host. The ChronoSphere player rewinds their hand back to before you existed. The Neural Gambit player boosts a 2 into a 14. The Deliverance player calmly returns a card to the deck and gains another charge.

You bust. The Host gets Blackjack. Round over. Skills shuffle. It happens again.

Welcome to CrazyJack.
