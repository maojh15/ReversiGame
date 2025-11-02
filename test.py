from rich.console import Console
cons = Console()

from rich.progress import Progress
import time

with Progress() as progress:
    task1 = progress.add_task("[red]Downloading...", total=1000)
    task2 = progress.add_task("[green]Processing...", total=1000)
    for i in range(1000):
        progress.advance(task1)
        progress.advance(task2)
        time.sleep(0.01)
