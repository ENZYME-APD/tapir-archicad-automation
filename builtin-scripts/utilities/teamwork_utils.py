from mu

class TeamworkReserve:
    def __init__(self, conn_header: ConnHeader, elements: list[Guid]):
        self.core: CoreCommands = conn_header.core
        self.is_teamwork: bool = isinstance(conn_header.archicad_id, TeamworkProjectID)
        self.elements: list[Guid] = elements

    def __enter__(self):
        if self.is_teamwork and self.elements:
            self.core.post_tapir_command(
                command="ReserveElements", parameters={"elements": self.elements}
            )
            # TODO: If post_tapir_command returns a status/conflicts, check it here
        return self

    def __exit__(self, exc_type: Optional[type], exc_val: Optional[BaseException], exc_tb: Optional[Any]):
        if self.is_teamwork  and self.elements:
            self.core.post_tapir_command(
                command="ReleaseElements", parameters={"elements": self.elements}
            )
        return False