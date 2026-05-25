const cards = document.querySelectorAll("card")
for (const card of cards) {
    card.onclick = () => card.classList.toggle("flipped")
}

const stylesheet = document.createElement("style")
stylesheet.textContent = `
    card { 
        cursor: pointer;
        padding:0.2rem;
        background-color:rgba(100,100,100,0.2);
        border-radius:0.5rem;
        transition: background-color 0.1s;
    }
    card.flipped{background-color:rgba(100,100,100,0.5);}
    card back{display:none}
    card.flipped front{display:none}
    card.flipped back{display:inline-block}
`
document.body.appendChild(stylesheet)